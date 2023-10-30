import 'dart:io';

import 'package:flutter/material.dart';
import 'dart:async';

import 'package:foxglove/foxglove.dart';
import 'package:logging/logging.dart';

void main() {
  Logger.root.level = Level.ALL;
  Logger.root.onRecord.listen((record) {
    print('${record.level.name}: ${record.time}: ${record.message}');
  });
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  Player? _player;

  @override
  void initState() {
    super.initState();
    initPlatformState();
  }

  Future<void> initPlatformState() async {
    final player = await PlayerPlatform.instance
        .createPlayer(environmentArgs: ['--no-osd']);
    _player = player;

    if (mounted) {
      setState(() {});
    }

    final m = Media.file(File(r'some_video.mp4'));
    await player.open(m);
    await player.setPlaylistMode(PlaylistMode.loop);
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Plugin example app'),
        ),
        body: Center(
            child:
                _player == null ? const SizedBox() : Video(player: _player!)),
      ),
    );
  }
}
