import 'dart:async';

import 'package:flutter/services.dart';
import 'package:foxglove/foxglove.dart';

const MethodChannel _channel = MethodChannel('foxglove');

abstract class Player {
  Future<void> dispose();
  Future<void> open(MediaSource media);
  Future<void> play();
}

class _PlayerImpl implements Player {
  final PlayerPlatform platform;
  final int id;
  final MethodChannel _methodChannel;
  _PlayerImpl({required this.platform, required this.id})
      : _methodChannel = MethodChannel('foxglove/$id');

  //Future<void> play() {
  //  PlayerPlatform._channel.invokeMethod('player');
  //}

  @override
  Future<void> dispose() async {
    await _channel.invokeMethod('disposePlayer', id);
  }

  @override
  Future<void> open(MediaSource source) async {
    if (source is Media) {
      await _methodChannel.invokeMethod('open', {
        'type': 'media',
        'media': {'type': source.mediaType.name, 'resource': source.resource}
      });
    }
  }

  @override
  Future<void> play() async {
    await _methodChannel.invokeMethod('play');
  }
}

class PlayerPlatform {
  /// Attempts to create an enviroment with the given [args]
  Future<int> createEnvironment({List<String>? args}) async {
    final envId = await _channel.invokeMethod<int>(
        'createEnvironment', <String, dynamic>{'args': args}) as int;
    return envId;
  }

  Future<void> disposeEnvironment(int environmentId) async {
    await _channel.invokeMethod('disposeEnvironment', environmentId);
  }

  /// Attempts to create a new player.
  Future<Player?> createPlayer({int? environmentId}) async {
    final result = await _channel.invokeMapMethod(
        'createPlayer', <String, dynamic>{'environmentId': environmentId});

    final playerId = result!['player_id'] as int;
    return _PlayerImpl(platform: this, id: playerId);
  }
}
