"""Config flow for HA Box integration."""

from homeassistant import config_entries
from .const import DOMAIN, DEVICE_NAME


class HABoxConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    """Handle the config flow for HA Box."""

    VERSION = 1

    async def async_step_user(self, user_input=None):
        await self.async_set_unique_id(DOMAIN)
        self._abort_if_unique_id_configured()
        return self.async_create_entry(title=DEVICE_NAME, data={})
