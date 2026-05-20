"""Configuration management for HA Box."""

import logging
import os
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional

logger = logging.getLogger(__name__)

def load_options() -> dict:
    """
    Load options from Home Assistant App configuration.
    
    Returns:
        Dictionary with configuration options
    """
    try:
        import json
        options_file = os.environ.get("SUPERVISOR_OPTIONS", "/data/options.json")
        if os.path.exists(options_file):
            with open(options_file, "r") as f:
                return json.load(f)
        else:
            logger.warning(f"Options file not found: {options_file}")
            return {}
    except Exception as e:
        logger.error(f"Error loading options: {e}")
        return {}


def _box_options_from_legacy(options: dict) -> dict:
    """
    Build a ``box``-shaped dict from pre-0.2.x top-level keys.

    Used when ``options`` does not contain a ``box`` key so existing installs
    keep working until the user migrates the YAML.
    """
    esp32 = options.get("esp32") or {}
    ha = options.get("home_assistant") or {}
    sens = options.get("sensors") or {}
    return {
        "lang": "auto",
        "weather": {
            "weather_entity": ha.get("weather_entity", "weather.forecast_home"),
            "update_interval": ha.get("update_interval", 300),
        },
        "bme280": sens.get("bme280") or {},
        "fan": esp32.get("fan") or {},
        "ota": esp32.get("ota") or {},
    }


@dataclass
class WeatherConfig:
    """Weather entity and polling (data sent to ESP32 display)."""

    weather_entity: str = "weather.forecast_home"
    update_interval: int = 300


@dataclass
class BME280Config:
    """BME280 sensor configuration."""
    enabled: bool = True
    address: str = "auto"  # "auto", "0x76", or "0x77"
    update_interval: int = 30  # seconds

@dataclass
class FanConfig:
    """Fan control configuration (sent to ESP32 on connection via FAN verb)."""
    enabled: bool = False
    min_temp: float = 28.0  # tOn  – fan starts at this temperature (°C)
    max_temp: float = 60.0  # tFull – fan at full speed at this temperature (°C)

@dataclass
class OTAConfig:
    """OTA firmware update configuration (GPIO pins for bootloader entry)."""
    io0_gpio: Optional[int] = None   # Pi BCM GPIO connected to ESP32 IO0
    en_gpio: Optional[int] = None    # Pi BCM GPIO connected to ESP32 EN
    force_flash: bool = False        # Skip version check and always flash on connect

    @property
    def is_available(self) -> bool:
        """True if both GPIO pins are configured."""
        return self.io0_gpio is not None and self.en_gpio is not None

@dataclass
class DayNightConfig:
    """Day/night mode synchronisation configuration."""
    update_interval: int = 60  # seconds between sun.sun polls


@dataclass
class ClockConfig:
    """Clock synchronisation configuration."""
    update_interval: int = 60  # seconds between CLOCK sends to ESP32


@dataclass
class StatusConfig:
    """Status handler configuration."""
    update_interval: int = 30  # seconds between STATUS sends to ESP32
    exposed_addon_slugs: List[str] = field(default_factory=list)


@dataclass
class UARTConfig:
    """UART serial configuration."""
    device: str = "auto"  # "auto" = scan DEFAULT_DEVICE_PATTERNS, else explicit path


@dataclass
class ConnectionConfig:
    """ESP32 connection state-machine timeouts."""
    connected_timeout: int = 120    # seconds before CONNECTED → RECONNECTING
    reconnecting_timeout: int = 60  # seconds before RECONNECTING → DISCONNECTED


@dataclass
class RF433Config:
    """CC1101 433MHz module configuration (SPI on Pi, RFLink TCP server)."""
    enabled: bool = False
    tcp_port: int = 5557       # TCP port for RFLink-compatible server
    spi_bus: int = 0           # SPI bus (SPI0)
    spi_device: int = 0        # SPI device (CE0 = GPIO8)
    gdo0_pin: int = 17         # CC1101 GDO0 → Pi GPIO17 (pin 11)
    gdo2_pin: int = 27         # CC1101 GDO2 → Pi GPIO27 (pin 13)


@dataclass
class BoxConfig:
    """Box / ESP32 stack: language, weather, sensors, fan, OTA."""

    log_level: str = "info"  # debug, info, warning, error, critical
    lang: str = "auto"  # auto, fr, en
    weather: WeatherConfig = field(default_factory=WeatherConfig)
    bme280: BME280Config = field(default_factory=BME280Config)
    fan: FanConfig = field(default_factory=FanConfig)
    ota: OTAConfig = field(default_factory=OTAConfig)
    daynight: DayNightConfig = field(default_factory=DayNightConfig)
    clock: ClockConfig = field(default_factory=ClockConfig)
    status: StatusConfig = field(default_factory=StatusConfig)
    uart: UARTConfig = field(default_factory=UARTConfig)
    connection: ConnectionConfig = field(default_factory=ConnectionConfig)
    rf433: RF433Config = field(default_factory=RF433Config)


@dataclass
class Config:
    """Main configuration class."""

    box: BoxConfig = field(default_factory=BoxConfig)

    @classmethod
    def from_options(cls, options: dict) -> "Config":
        """Create Config from Home Assistant options."""
        config = cls()
        box_opts: Dict[str, Any]
        if "general" in options:
            # Current format: log_level/lang wrapped in a "general" group
            box_opts = options
        elif any(k in options for k in ("weather", "bme280", "fan", "ota", "log_level", "lang")):
            # Previous flat format: sub-groups at root level
            box_opts = options
        elif "box" in options:
            # Legacy 0.2.x format: sub-groups wrapped in a "box" key
            box_opts = dict(options["box"] or {})
        else:
            # Pre-0.2.x format: completely different key names
            box_opts = _box_options_from_legacy(options)

        g = box_opts.get("general") or {}
        w = box_opts.get("weather") or {}
        b = box_opts.get("bme280") or {}
        f = box_opts.get("fan") or {}
        o = box_opts.get("ota") or {}
        dn = box_opts.get("daynight") or {}
        cl = box_opts.get("clock") or {}
        st = box_opts.get("status") or {}
        ua = box_opts.get("uart") or {}
        co = box_opts.get("connection") or {}
        rf = box_opts.get("rf433") or {}

        log_level_raw = g.get("log_level") or box_opts.get("log_level", "info")
        log_level = log_level_raw.lower().strip() if isinstance(log_level_raw, str) else "info"
        if log_level not in ("trace", "debug", "info", "warning", "error", "critical"):
            logger.warning("Invalid box.log_level=%r, using info", log_level_raw)
            log_level = "info"

        lang_raw = g.get("lang") or box_opts.get("lang", "auto")
        if isinstance(lang_raw, str):
            lang = lang_raw.lower().strip()
        else:
            lang = "auto"
        if lang not in ("auto", "fr", "en"):
            logger.warning("Invalid box.lang=%r, using auto", lang_raw)
            lang = "auto"

        config.box = BoxConfig(
            log_level=log_level,
            lang=lang,
            weather=WeatherConfig(
                weather_entity=w.get("weather_entity", "weather.forecast_home"),
                update_interval=int(w.get("update_interval", 300)),
            ),
            bme280=BME280Config(
                enabled=b.get("enabled", True),
                address=b.get("address", "auto"),
                update_interval=int(b.get("update_interval", 30)),
            ),
            fan=FanConfig(
                enabled=f.get("enabled", False),
                min_temp=float(f.get("min_temp", 28)),
                max_temp=float(f.get("max_temp", 60)),
            ),
            ota=OTAConfig(
                io0_gpio=int(o["io0_gpio"]) if o.get("io0_gpio") is not None else None,
                en_gpio=int(o["en_gpio"]) if o.get("en_gpio") is not None else None,
                force_flash=bool(o.get("force_flash", False)),
            ),
            daynight=DayNightConfig(
                update_interval=int(dn.get("update_interval", 60)),
            ),
            clock=ClockConfig(
                update_interval=int(cl.get("update_interval", 60)),
            ),
            status=StatusConfig(
                update_interval=int(st.get("update_interval", 30)),
                exposed_addon_slugs=list(st.get("exposed_addon_slugs") or []),
            ),
            uart=UARTConfig(
                device=str(ua.get("device", "auto")),
            ),
            connection=ConnectionConfig(
                connected_timeout=int(co.get("connected_timeout", 120)),
                reconnecting_timeout=int(co.get("reconnecting_timeout", 60)),
            ),
            rf433=RF433Config(
                enabled=bool(rf.get("enabled", False)),
                tcp_port=int(rf.get("tcp_port", 5557)),
                spi_bus=int(rf.get("spi_bus", 0)),
                spi_device=int(rf.get("spi_device", 0)),
                gdo0_pin=int(rf.get("gdo0_pin", 17)),
                gdo2_pin=int(rf.get("gdo2_pin", 27)),
            ),
        )
        return config
