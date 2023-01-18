#include "include/foxglove/foxglove_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "foxglove_plugin.h"

void FoxglovePluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  foxglove::windows::FoxglovePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
