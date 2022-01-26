import 'dart:async';

import 'package:flutter/services.dart';
import 'package:foxglove/foxglove.dart';

const MethodChannel _channel = MethodChannel('foxglove');

extension on Media {
  Map<String, dynamic> toJson() {
    return {'type': mediaType.name, 'resource': resource};
  }
}

extension on Playlist {
  Map<String, dynamic> toJson() {
    final medias = this.medias.map((m) => m.toJson()).toList();
    return {'medias': medias, 'mode': playlistMode.index};
  }
}

abstract class Player {
  Future<void> dispose();
  Future<void> open(MediaSource media);
  Future<void> setPlaylistMode(PlaylistMode playlistMode);
  Future<void> play();
  Future<void> pause();
  Future<void> stop();
  Future<void> next();
  Future<void> previous();
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
      await _methodChannel.invokeMethod('open', {'media': source.toJson()});
    } else if (source is Playlist) {
      await _methodChannel.invokeMethod('open', {'playlist': source.toJson()});
    }
  }

  @override
  Future<void> setPlaylistMode(PlaylistMode mode) async {
    await _methodChannel.invokeMethod('setPlaylistMode', mode.index);
  }

  @override
  Future<void> play() async {
    await _methodChannel.invokeMethod('play');
  }

  @override
  Future<void> pause() async {
    await _methodChannel.invokeMethod('pause');
  }

  @override
  Future<void> stop() async {
    await _methodChannel.invokeMethod('stop');
  }

  @override
  Future<void> next() async {
    await _methodChannel.invokeMethod('next');
  }

  @override
  Future<void> previous() async {
    await _methodChannel.invokeMethod('previous');
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
