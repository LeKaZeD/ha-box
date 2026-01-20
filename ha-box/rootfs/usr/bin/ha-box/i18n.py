"""
Internationalization (i18n) module for HA Box add-on.

This module provides translation support for user-facing messages in Python code.
Home Assistant automatically handles translations for config.yaml labels/descriptions
via the translations/ folder, but this module is needed for runtime messages.
"""

import os
import yaml
import logging
from pathlib import Path
from typing import Dict, Optional

logger = logging.getLogger(__name__)

# Default language
DEFAULT_LANGUAGE = "en"

# Supported languages
SUPPORTED_LANGUAGES = ["en", "fr"]


class Translator:
    """Translation manager for HA Box add-on."""

    def __init__(self, language: Optional[str] = None) -> None:
        """
        Initialize the translator.

        Args:
            language: Language code (e.g., 'en', 'fr'). If None, auto-detects
                     from environment or falls back to DEFAULT_LANGUAGE.
        """
        self._translations: Dict[str, str] = {}
        self._language = self._detect_language(language)
        self._load_translations()

    def _detect_language(self, language: Optional[str]) -> str:
        """
        Detect the language to use.

        Args:
            language: Explicit language code, or None for auto-detection.

        Returns:
            Language code string.
        """
        if language:
            if language in SUPPORTED_LANGUAGES:
                return language
            logger.warning(
                "Unsupported language '%s', falling back to %s",
                language,
                DEFAULT_LANGUAGE,
            )
            return DEFAULT_LANGUAGE

        # Try to detect from environment
        env_lang = os.environ.get("LANG", "")
        if env_lang:
            # Extract language code (e.g., "fr_FR.UTF-8" -> "fr")
            lang_code = env_lang.split("_")[0].lower()
            if lang_code in SUPPORTED_LANGUAGES:
                return lang_code

        # Try Home Assistant language setting (if available)
        ha_lang = os.environ.get("SUPERVISOR_LANGUAGE", "")
        if ha_lang:
            lang_code = ha_lang.lower()
            if lang_code in SUPPORTED_LANGUAGES:
                return lang_code

        return DEFAULT_LANGUAGE

    def _load_translations(self) -> None:
        """Load translation file for the current language."""
        # Translation files are in the add-on root, but we run from /usr/bin/ha-box
        # Need to find the add-on root
        addon_root = self._find_addon_root()
        if not addon_root:
            logger.warning(
                "Could not find add-on root, translations will not be loaded"
            )
            return

        translation_file = addon_root / "translations" / f"{self._language}.yaml"
        if not translation_file.exists():
            logger.warning(
                "Translation file not found: %s, falling back to English",
                translation_file,
            )
            if self._language != DEFAULT_LANGUAGE:
                translation_file = addon_root / "translations" / f"{DEFAULT_LANGUAGE}.yaml"
            if not translation_file.exists():
                logger.warning("English translation file not found, no translations loaded")
                return

        try:
            with open(translation_file, "r", encoding="utf-8") as f:
                data = yaml.safe_load(f)
                # Extract messages section if it exists
                # For now, we'll use a simple flat structure
                # Future: add 'messages' section to translation files
                if isinstance(data, dict) and "messages" in data:
                    self._translations = data["messages"]
                else:
                    # No messages section yet, empty translations
                    self._translations = {}
        except Exception as e:
            logger.error("Failed to load translations from %s: %s", translation_file, e)
            self._translations = {}

    def _find_addon_root(self) -> Optional[Path]:
        """
        Find the add-on root directory.

        Returns:
            Path to add-on root, or None if not found.
        """
        # Translation files are copied to rootfs during build
        # They should be at /addon/translations/ in the container
        # But we can also check relative to our code location
        possible_paths = [
            Path("/addon"),  # Standard add-on root in container
            Path("/data"),   # Alternative location
            Path(__file__).parent.parent.parent.parent / "ha-box",  # Development path
        ]

        for path in possible_paths:
            translations_dir = path / "translations"
            if translations_dir.exists() and translations_dir.is_dir():
                return path

        # Fallback: check if translations are in the same directory structure
        # as the code (for development/testing)
        code_dir = Path(__file__).parent
        # Go up: /usr/bin/ha-box -> /usr/bin -> /usr -> /
        # Then look for ha-box/translations
        root = code_dir.parent.parent.parent
        translations_dir = root / "ha-box" / "translations"
        if translations_dir.exists() and translations_dir.is_dir():
            return root / "ha-box"

        return None

    def get(self, key: str, default: Optional[str] = None) -> str:
        """
        Get a translated message.

        Args:
            key: Translation key (e.g., "common.temperature")
            default: Default value if translation not found. If None, returns the key.

        Returns:
            Translated string, or default/key if not found.
        """
        value = self._translations.get(key)
        if value:
            return value

        if default is not None:
            return default

        # Return key as fallback
        logger.debug("Translation not found for key: %s", key)
        return key

    @property
    def language(self) -> str:
        """Get the current language code."""
        return self._language


# Global translator instance (lazy initialization)
_translator: Optional[Translator] = None


def get_translator(language: Optional[str] = None) -> Translator:
    """
    Get the global translator instance.

    Args:
        language: Optional language code to initialize with.

    Returns:
        Translator instance.
    """
    global _translator
    if _translator is None:
        _translator = Translator(language)
    return _translator


def translate(key: str, default: Optional[str] = None) -> str:
    """
    Convenience function to translate a key.

    Args:
        key: Translation key.
        default: Default value if translation not found.

    Returns:
        Translated string.
    """
    return get_translator().get(key, default)
