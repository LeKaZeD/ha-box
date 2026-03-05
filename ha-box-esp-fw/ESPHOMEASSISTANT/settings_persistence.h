#pragma once

struct Model;

void loadSettings(Model& model);
void saveSettingsIfChanged(const Model& model);
