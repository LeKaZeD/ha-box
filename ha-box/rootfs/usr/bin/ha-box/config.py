"""Configuration management for HA Box."""

import logging
import os
from dataclasses import dataclass
from typing import List, Optional

logger = logging.getLogger(__name__)

def load_options() -> dict:
    """
    Load options from Home Assistant add-on configuration.
    
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

@dataclass
class FrontLightConfig:
    """Front-light configuration."""
    enabled: bool = True
    brightness: int = 50  # 0-100

@dataclass
class DisplayConfig:
    """Display configuration."""
    enabled: bool = True
    rotation: int = 0
    refresh_mode: str = "full"  # "full" or "partial"
    front_light: FrontLightConfig = None
    
    def __post_init__(self):
        if self.front_light is None:
            self.front_light = FrontLightConfig()

@dataclass
class BME280Config:
    """BME280 sensor configuration."""
    enabled: bool = True
    address: str = "auto"  # "auto", "0x76", or "0x77"
    update_interval: int = 30  # seconds

@dataclass
class NFCConfig:
    """NFC sensor configuration."""
    enabled: bool = True
    address: str = "0x24"
    polling_interval: int = 1  # seconds

@dataclass
class TouchConfig:
    """Touch sensor configuration."""
    enabled: bool = True

@dataclass
class SensorsConfig:
    """Sensors configuration."""
    bme280: BME280Config = None
    nfc: NFCConfig = None
    touch: TouchConfig = None
    
    def __post_init__(self):
        if self.bme280 is None:
            self.bme280 = BME280Config()
        if self.nfc is None:
            self.nfc = NFCConfig()
        if self.touch is None:
            self.touch = TouchConfig()

@dataclass
class FanConfig:
    """Fan control configuration."""
    enabled: bool = False
    pin: int = 18
    auto_control: bool = True
    min_temp: float = 40.0  # Celsius
    max_temp: float = 60.0  # Celsius

@dataclass
class LEDConfig:
    """LED strip configuration."""
    enabled: bool = False
    pin: int = 21
    count: int = 10

@dataclass
class ControlConfig:
    """Control devices configuration."""
    fan: FanConfig = None
    led: LEDConfig = None
    
    def __post_init__(self):
        if self.fan is None:
            self.fan = FanConfig()
        if self.led is None:
            self.led = LEDConfig()

@dataclass
class HomeAssistantConfig:
    """Home Assistant integration configuration."""
    entities: List[str] = None
    update_interval: int = 60  # seconds
    
    def __post_init__(self):
        if self.entities is None:
            self.entities = []

@dataclass
class Config:
    """Main configuration class."""
    display: DisplayConfig = None
    sensors: SensorsConfig = None
    control: ControlConfig = None
    home_assistant: HomeAssistantConfig = None
    
    def __post_init__(self):
        if self.display is None:
            self.display = DisplayConfig()
        if self.sensors is None:
            self.sensors = SensorsConfig()
        if self.control is None:
            self.control = ControlConfig()
        if self.home_assistant is None:
            self.home_assistant = HomeAssistantConfig()
    
    @classmethod
    def from_options(cls, options: dict) -> "Config":
        """Create Config from Home Assistant options."""
        config = cls()
        
        # Display config
        if "display" in options:
            display_opts = options["display"]
            config.display = DisplayConfig(
                enabled=display_opts.get("enabled", True),
                rotation=display_opts.get("rotation", 0),
                refresh_mode=display_opts.get("refresh_mode", "full"),
                front_light=FrontLightConfig(
                    enabled=display_opts.get("front_light", {}).get("enabled", True),
                    brightness=display_opts.get("front_light", {}).get("brightness", 50)
                )
            )
        
        # Sensors config
        if "sensors" in options:
            sensors_opts = options["sensors"]
            config.sensors = SensorsConfig(
                bme280=BME280Config(
                    enabled=sensors_opts.get("bme280", {}).get("enabled", True),
                    address=sensors_opts.get("bme280", {}).get("address", "auto"),
                    update_interval=sensors_opts.get("bme280", {}).get("update_interval", 30)
                ),
                nfc=NFCConfig(
                    enabled=sensors_opts.get("nfc", {}).get("enabled", True),
                    address=sensors_opts.get("nfc", {}).get("address", "0x24"),
                    polling_interval=sensors_opts.get("nfc", {}).get("polling_interval", 1)
                ),
                touch=TouchConfig(
                    enabled=sensors_opts.get("touch", {}).get("enabled", True)
                )
            )
        
        # Control config
        if "control" in options:
            control_opts = options["control"]
            config.control = ControlConfig(
                fan=FanConfig(
                    enabled=control_opts.get("fan", {}).get("enabled", False),
                    pin=control_opts.get("fan", {}).get("pin", 18),
                    auto_control=control_opts.get("fan", {}).get("auto_control", True),
                    min_temp=control_opts.get("fan", {}).get("min_temp", 40),
                    max_temp=control_opts.get("fan", {}).get("max_temp", 60)
                ),
                led=LEDConfig(
                    enabled=control_opts.get("led", {}).get("enabled", False),
                    pin=control_opts.get("led", {}).get("pin", 21),
                    count=control_opts.get("led", {}).get("count", 10)
                )
            )
        
        # Home Assistant config
        if "home_assistant" in options:
            ha_opts = options["home_assistant"]
            config.home_assistant = HomeAssistantConfig(
                entities=ha_opts.get("entities", []),
                update_interval=ha_opts.get("update_interval", 60)
            )
        
        return config
