"""Logging configuration for HA Box."""

import logging
import sys

TRACE = 5
logging.addLevelName(TRACE, "TRACE")

_LEVEL_MAP = {
    "trace": TRACE,
    "debug": logging.DEBUG,
    "info": logging.INFO,
    "warning": logging.WARNING,
    "error": logging.ERROR,
    "critical": logging.CRITICAL,
}


def parse_log_level(level_str: str) -> int:
    """Convert a level name string to a logging level int. Defaults to INFO."""
    return _LEVEL_MAP.get(level_str.lower(), logging.INFO)


def setup_logging(level: int = logging.INFO) -> logging.Logger:
    """
    Setup logging configuration.
    
    Args:
        level: Logging level (default: INFO)
    
    Returns:
        Configured logger instance
    """
    # Configure root logger to catch all loggers
    root_logger = logging.getLogger()
    root_logger.setLevel(level)
    
    # Remove existing handlers to avoid duplicates
    for handler in root_logger.handlers[:]:
        root_logger.removeHandler(handler)
    
    # Create console handler
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(level)
    
    # Create formatter
    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )
    handler.setFormatter(formatter)
    
    # Add handler to root logger
    root_logger.addHandler(handler)
    
    # Return main logger
    logger = logging.getLogger("ha-box")
    return logger
