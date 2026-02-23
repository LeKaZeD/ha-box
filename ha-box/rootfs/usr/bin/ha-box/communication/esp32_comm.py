"""
ESP32 communication module for HA Box add-on.

This module implements the ASCII protocol for communication with the ESP32
via UART (Serial0). It handles message parsing, ACK management, and
deduplication.
"""

import logging
import re
import serial
import serial.tools.list_ports
import threading
import time
from dataclasses import dataclass
from typing import Callable, Dict, List, Optional, Tuple

logger = logging.getLogger(__name__)

# Protocol constants (matching AsciiProto.h)
MAX_LINE = 256
MAX_VERB = 24
MAX_KV = 12
MAX_KEY = 24
MAX_VAL = 96
DEDUPE_WINDOW = 8

# Default serial configuration
DEFAULT_BAUDRATE = 115200
DEFAULT_TIMEOUT = 1.0
DEFAULT_DEVICE_PATTERNS = ["/dev/serial0", "/dev/ttyAMA0", "/dev/ttyUSB0"]


@dataclass
class KV:
    """Key-value pair for message parameters."""

    key: str
    val: str


@dataclass
class Message:
    """Incoming message from ESP32."""

    id: int
    verb: str
    kv: List[KV]
    kv_count: int = 0

    def __post_init__(self) -> None:
        """Initialize kv_count from kv list."""
        if self.kv_count == 0:
            self.kv_count = len(self.kv)

    def get(self, key: str, default: Optional[str] = None) -> Optional[str]:
        """
        Get a value by key.

        Args:
            key: Key to look up.
            default: Default value if key not found.

        Returns:
            Value string or default.
        """
        for kv in self.kv:
            if kv.key == key:
                return kv.val
        return default


@dataclass
class Ack:
    """ACK response from ESP32."""

    id: int
    ok: bool
    err: str = ""


class ESP32Comm:
    """
    ESP32 communication handler using ASCII protocol over UART.

    This class manages bidirectional communication with the ESP32,
    handling message parsing, ACK management, and deduplication.
    """

    def __init__(
        self,
        device: Optional[str] = None,
        baudrate: int = DEFAULT_BAUDRATE,
        timeout: float = DEFAULT_TIMEOUT,
    ) -> None:
        """
        Initialize ESP32 communication handler.

        Args:
            device: Serial device path (e.g., "/dev/serial0").
                   If None, auto-detects from common patterns.
            baudrate: Serial baudrate (default: 115200).
            timeout: Serial read timeout in seconds (default: 1.0).
        """
        self._device = device or self._find_serial_device()
        self._baudrate = baudrate
        self._timeout = timeout
        self._serial: Optional[serial.Serial] = None

        # Threading
        self._running = False
        self._thread: Optional[threading.Thread] = None
        self._lock = threading.Lock()

        # Protocol state
        self._next_id = 1
        self._last_seen = 0.0
        self._recent_ids: List[int] = [0] * DEDUPE_WINDOW
        self._recent_pos = 0

        # Callbacks
        self._auth_callback: Optional[Callable[[Message], Tuple[bool, Optional[str]]]] = None
        self._message_callback: Optional[Callable[[Message], None]] = None
        self._ack_callback: Optional[Callable[[Ack], None]] = None

        # Synchronous ACK waiting
        self._waiting_ack: Dict[int, Optional[Ack]] = {}
        self._ack_events: Dict[int, threading.Event] = {}

        # Line buffer
        self._line_buffer = bytearray()

        logger.info(
            "ESP32Comm initialized: device=%s, baudrate=%d",
            self._device,
            self._baudrate,
        )

    def _find_serial_device(self) -> str:
        """
        Auto-detect serial device.

        Returns:
            Device path, or raises RuntimeError if not found.
        """
        # Try common patterns
        for pattern in DEFAULT_DEVICE_PATTERNS:
            try:
                # Check if device exists
                import os

                if os.path.exists(pattern):
                    logger.info("Found serial device: %s", pattern)
                    return pattern
            except Exception as e:
                logger.debug("Error checking device %s: %s", pattern, e)

        # Try to list serial ports
        try:
            ports = serial.tools.list_ports.comports()
            if ports:
                device = ports[0].device
                logger.info("Auto-detected serial device: %s", device)
                return device
        except Exception as e:
            logger.debug("Error listing serial ports: %s", e)

        raise RuntimeError(
            f"Could not find serial device. Tried: {', '.join(DEFAULT_DEVICE_PATTERNS)}"
        )

    def begin(self) -> None:
        """
        Start the communication handler.

        Opens the serial port and starts the polling thread.
        """
        if self._running:
            logger.warning("ESP32Comm already started")
            return

        try:
            logger.info("Opening serial port: %s @ %d", self._device, self._baudrate)
            self._serial = serial.Serial(
                port=self._device,
                baudrate=self._baudrate,
                timeout=self._timeout,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                xonxoff=False,
                rtscts=False,
                dsrdtr=False,
            )
            # Clear any stale data in buffers
            self._serial.reset_input_buffer()
            self._serial.reset_output_buffer()
            logger.info("Serial port opened: %s (baudrate=%d)", self._device, self._baudrate)
        except Exception as e:
            logger.error("Failed to open serial port %s: %s", self._device, e)
            raise

        self._running = True
        # Don't set _last_seen here, only when we actually receive a message
        self._thread = threading.Thread(target=self._poll_loop, daemon=True)
        self._thread.start()
        logger.info("ESP32Comm polling thread started")

    def stop(self) -> None:
        """
        Stop the communication handler.

        Closes the serial port and stops the polling thread.
        """
        if not self._running:
            return

        logger.info("Stopping ESP32Comm...")
        self._running = False

        if self._thread:
            self._thread.join(timeout=2.0)

        if self._serial and self._serial.is_open:
            self._serial.close()
            logger.info("Serial port closed")

    def last_seen_ms(self) -> float:
        """
        Get time since last valid message (in seconds).

        Returns:
            Time in seconds since last valid message, or -1 if never seen.
        """
        if self._last_seen == 0.0:
            return -1.0  # Never seen
        return time.time() - self._last_seen
    
    def has_seen_esp32(self) -> bool:
        """
        Check if we have ever received a message from ESP32.
        
        Returns:
            True if at least one message was received, False otherwise.
        """
        return self._last_seen > 0.0

    def set_authorize_callback(
        self, callback: Callable[[Message], Tuple[bool, Optional[str]]]
    ) -> None:
        """
        Set callback for message authorization.

        Args:
            callback: Function that takes a Message and returns (ok, err_code).
                     If ok is False, an ACK ERR is sent with err_code.
        """
        self._auth_callback = callback

    def set_message_callback(self, callback: Callable[[Message], None]) -> None:
        """
        Set callback for incoming messages.

        Args:
            callback: Function that receives a Message.
        """
        self._message_callback = callback

    def set_ack_callback(self, callback: Callable[[Ack], None]) -> None:
        """
        Set callback for incoming ACKs.

        Args:
            callback: Function that receives an Ack.
        """
        self._ack_callback = callback

    def next_id(self) -> int:
        """
        Get next message ID.

        Returns:
            Next unique message ID.
        """
        with self._lock:
            msg_id = self._next_id
            self._next_id += 1
            return msg_id

    def send_command(
        self, verb: str, kv: Optional[List[KV]] = None, kv_count: Optional[int] = None
    ) -> int:
        """
        Send a command to ESP32.

        Args:
            verb: Command verb (e.g., "READY", "STATUS").
            kv: Optional list of key-value pairs.
            kv_count: Optional count (if None, uses len(kv)).

        Returns:
            Message ID of the sent command.
        """
        if not self._serial or not self._serial.is_open:
            logger.error("Serial port not open")
            return 0

        msg_id = self.next_id()
        kv = kv or []
        kv_count = kv_count if kv_count is not None else len(kv)

        # Build message line: <id> <VERB> [key=value]...
        parts = [str(msg_id), verb]
        logger.debug("Building command: verb=%s, kv_count=%d, kv=%s", verb, kv_count, kv)
        
        for i in range(min(kv_count, len(kv))):
            if kv[i].key:
                parts.append(f"{kv[i].key}={kv[i].val}")
                logger.debug("  Added KV: %s=%s", kv[i].key, kv[i].val)

        line = " ".join(parts)
        logger.debug("Command line built: '%s' (length=%d)", line, len(line))
        
        if len(line) > MAX_LINE:
            logger.warning("Message too long, truncating: %d bytes", len(line))
            line = line[: MAX_LINE - 1]

        try:
            data = (line + "\n").encode("ascii")
            logger.debug("Sending command: line='%s', encoded=%s, length=%d bytes", 
                        line, data.hex(), len(data))
            self._serial.write(data)
            self._serial.flush()  # Ensure data is sent immediately
            logger.info("TX: %s", line)
            logger.debug("UART TX raw bytes: %s", data.hex())
            logger.debug("Command sent successfully, msg_id=%d", msg_id)
        except Exception as e:
            logger.error("Failed to send command: %s", e, exc_info=True)
            return 0

        return msg_id

    def send_with_ack(
        self,
        verb: str,
        kv: Optional[List[KV]] = None,
        kv_count: Optional[int] = None,
        ack_timeout_ms: int = 5000,
        retry_count: int = 3,
    ) -> Tuple[bool, Optional[Ack]]:
        """
        Send a command and wait for ACK (synchronous, blocking).

        Args:
            verb: Command verb.
            kv: Optional list of key-value pairs.
            kv_count: Optional count.
            ack_timeout_ms: Timeout in milliseconds to wait for ACK.
            retry_count: Number of retry attempts.

        Returns:
            Tuple of (success, Ack or None).
        """
        for attempt in range(retry_count + 1):
            msg_id = self.send_command(verb, kv, kv_count)
            if msg_id == 0:
                continue

            # Create event for this ACK
            event = threading.Event()
            with self._lock:
                self._ack_events[msg_id] = event
                self._waiting_ack[msg_id] = None

            # Wait for ACK
            timeout_sec = ack_timeout_ms / 1000.0
            if event.wait(timeout=timeout_sec):
                # ACK received
                with self._lock:
                    ack = self._waiting_ack.get(msg_id)
                    del self._waiting_ack[msg_id]
                    del self._ack_events[msg_id]

                if ack and ack.ok:
                    return True, ack
                # ACK was error, retry
                logger.debug("ACK error, retrying: %s", ack.err if ack else "unknown")
            else:
                # Timeout
                with self._lock:
                    if msg_id in self._waiting_ack:
                        del self._waiting_ack[msg_id]
                    if msg_id in self._ack_events:
                        del self._ack_events[msg_id]
                logger.debug("ACK timeout, retrying")

        return False, None

    def send_ack_ok(self, msg_id: int) -> None:
        """
        Send ACK OK for a message.

        Args:
            msg_id: Message ID to acknowledge.
        """
        if not self._serial or not self._serial.is_open:
            return

        line = f"ACK {msg_id} OK"
        try:
            data = (line + "\n").encode("ascii")
            self._serial.write(data)
            logger.debug("TX ACK OK: %d", msg_id)
        except Exception as e:
            logger.error("Failed to send ACK OK: %s", e)

    def send_ack_err(self, msg_id: int, err_code: str = "err") -> None:
        """
        Send ACK ERR for a message.

        Args:
            msg_id: Message ID to acknowledge.
            err_code: Error code string.
        """
        if not self._serial or not self._serial.is_open:
            return

        # Truncate err_code if too long
        if len(err_code) > MAX_KEY - 1:
            err_code = err_code[: MAX_KEY - 1]

        line = f"ACK {msg_id} ERR {err_code}"
        try:
            data = (line + "\n").encode("ascii")
            self._serial.write(data)
            logger.warning("TX ACK ERR: %d, %s", msg_id, err_code)
        except Exception as e:
            logger.error("Failed to send ACK ERR: %s", e)

    def _poll_loop(self) -> None:
        """Main polling loop (runs in background thread)."""
        while self._running:
            try:
                if self._serial and self._serial.is_open:
                    # Read available data
                    if self._serial.in_waiting > 0:
                        bytes_waiting = self._serial.in_waiting
                        data = self._serial.read(bytes_waiting)
                        logger.debug("Serial in_waiting=%d, read %d bytes", bytes_waiting, len(data))
                        self._process_data(data)
                    else:
                        # No data available, sleep to avoid busy-wait
                        time.sleep(0.01)  # 10ms polling interval
                else:
                    time.sleep(0.01)  # Reduced from 0.1 to 0.01 for faster polling
            except Exception as e:
                logger.error("Error in poll loop: %s", e, exc_info=True)
                time.sleep(0.1)

    def _process_data(self, data: bytes) -> None:
        """
        Process incoming serial data.

        Args:
            data: Raw bytes from serial port.
        """
        # Log raw received data for debugging (only in debug mode)
        logger.debug("UART RX raw bytes (%d): %s", len(data), data.hex())
        
        for byte in data:
            if byte == ord("\r"):
                continue  # Ignore CR

            if byte == ord("\n"):
                if len(self._line_buffer) > 0:
                    # Process complete line
                    try:
                        line = self._line_buffer.decode("ascii").strip()
                        if line:
                            self._process_line(line)
                    except UnicodeDecodeError as e:
                        logger.warning("Invalid line encoding: %s", e)
                    finally:
                        self._line_buffer.clear()
                continue

            # Add to buffer
            if len(self._line_buffer) < MAX_LINE - 1:
                self._line_buffer.append(byte)
            else:
                # Line overflow: reset
                logger.warning("Line buffer overflow, resetting")
                self._line_buffer.clear()

    def _process_line(self, line: str) -> None:
        """
        Process a complete line.

        Args:
            line: Complete line string (without newline).
        """
        # Log received line
        logger.info("RX: %s", line)
        
        # Try to parse as ACK first
        ack = self._parse_ack(line)
        if ack:
            self._last_seen = time.time()

            # Handle synchronous ACK waiting
            with self._lock:
                if ack.id in self._waiting_ack:
                    self._waiting_ack[ack.id] = ack
                    if ack.id in self._ack_events:
                        self._ack_events[ack.id].set()

            # Call ACK callback
            if self._ack_callback:
                try:
                    self._ack_callback(ack)
                except Exception as e:
                    logger.error("Error in ACK callback: %s", e)
            return

        # Try to parse as message
        msg = self._parse_message(line)
        if msg:
            self._last_seen = time.time()

            # Check for duplicates
            if self._is_duplicate(msg.id):
                logger.debug("Duplicate message ignored: %d", msg.id)
                return
            self._remember_id(msg.id)

            # Auto-ACK with authorization
            self._auto_ack_message(msg)

            # Call message callback
            if self._message_callback:
                try:
                    self._message_callback(msg)
                except Exception as e:
                    logger.error("Error in message callback: %s", e)
            return

        # Invalid line: ignored
        logger.debug("Invalid line ignored: %s", line)

    def _parse_ack(self, line: str) -> Optional[Ack]:
        """
        Parse an ACK line.

        Format: "ACK <id> OK" or "ACK <id> ERR <code>"

        Args:
            line: Line to parse.

        Returns:
            Ack object or None if not an ACK.
        """
        parts = line.split()
        if len(parts) < 3 or parts[0] != "ACK":
            return None

        try:
            msg_id = int(parts[1])
        except ValueError:
            return None

        if parts[2] == "OK":
            return Ack(id=msg_id, ok=True, err="")
        elif parts[2] == "ERR":
            err_code = parts[3] if len(parts) > 3 else "err"
            return Ack(id=msg_id, ok=False, err=err_code)

        return None

    def _parse_message(self, line: str) -> Optional[Message]:
        """
        Parse a message line.

        Format: "<id> <VERB> [key=value]..."

        Args:
            line: Line to parse.

        Returns:
            Message object or None if invalid.
        """
        parts = line.split()
        if len(parts) < 2:
            return None

        try:
            msg_id = int(parts[0])
        except ValueError:
            return None

        if msg_id == 0:
            return None

        verb = parts[1]
        if len(verb) > MAX_VERB:
            verb = verb[:MAX_VERB]

        kv_list: List[KV] = []

        # Parse key=value pairs
        for part in parts[2:]:
            if len(kv_list) >= MAX_KV:
                break

            if "=" not in part:
                continue

            key, val = part.split("=", 1)
            if not key:
                continue

            # Truncate if needed
            key = key[: MAX_KEY - 1]
            val = val[: MAX_VAL - 1]

            kv_list.append(KV(key=key, val=val))

        return Message(id=msg_id, verb=verb, kv=kv_list, kv_count=len(kv_list))

    def _auto_ack_message(self, msg: Message) -> None:
        """
        Automatically send ACK for a message (with authorization check).

        Args:
            msg: Message to acknowledge.
        """
        if self._auth_callback:
            try:
                ok, err_code = self._auth_callback(msg)
                if ok:
                    self.send_ack_ok(msg.id)
                else:
                    self.send_ack_err(msg.id, err_code or "unauthorized")
            except Exception as e:
                logger.error("Error in auth callback: %s", e)
                self.send_ack_err(msg.id, "auth_error")
        else:
            # No auth callback: auto-allow
            self.send_ack_ok(msg.id)

    def _is_duplicate(self, msg_id: int) -> bool:
        """
        Check if message ID is a duplicate.

        Args:
            msg_id: Message ID to check.

        Returns:
            True if duplicate.
        """
        return msg_id in self._recent_ids

    def _remember_id(self, msg_id: int) -> None:
        """
        Remember a message ID for deduplication.

        Args:
            msg_id: Message ID to remember.
        """
        self._recent_ids[self._recent_pos] = msg_id
        self._recent_pos = (self._recent_pos + 1) % DEDUPE_WINDOW
