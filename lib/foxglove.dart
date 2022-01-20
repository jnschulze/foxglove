
import 'dart:async';

import 'package:flutter/services.dart';

class Foxglove {
  static const MethodChannel _channel = MethodChannel('foxglove');

  static Future<String?> get platformVersion async {
    final String? version = await _channel.invokeMethod('getPlatformVersion');
    return version;
  }
}
