"""
RF433 status fetcher — probes CC1101 via SPI to check if it is reachable.
"""

import logging
from typing import Any, Dict

logger = logging.getLogger(__name__)


def fetch_rf433(spi_bus: int, spi_device: int) -> Dict[str, Any]:
    """
    Probe CC1101 PARTNUM register via SPI.

    The CC1101 PARTNUM register (address 0x30, burst read = 0xF0|0x30) always
    returns 0x00 on a healthy device. Any other value or exception means the
    module is absent or mis-wired.

    Args:
        spi_bus: SPI bus number (0 for SPI0).
        spi_device: SPI device/CS number (0 for CE0).

    Returns:
        Dict with key ``rf433_ok`` (bool).
    """
    out: Dict[str, Any] = {"rf433_ok": False}
    try:
        import spidev
        spi = spidev.SpiDev()
        spi.open(spi_bus, spi_device)
        spi.max_speed_hz = 50000
        spi.mode = 0
        # Burst read of PARTNUM register: header byte 0xF0 + dummy byte
        resp = spi.xfer2([0xF0 | 0x30, 0x00])
        spi.close()
        out["rf433_ok"] = (resp[1] == 0x00)
        if out["rf433_ok"]:
            logger.debug("CC1101 detected on SPI%d.%d (PARTNUM=0x00)", spi_bus, spi_device)
        else:
            logger.debug("CC1101 SPI%d.%d: unexpected PARTNUM=0x%02x", spi_bus, spi_device, resp[1])
    except Exception as e:
        logger.debug("CC1101 SPI%d.%d probe failed: %s", spi_bus, spi_device, e)
    return out
