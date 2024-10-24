import 'dart:async';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:foxglove/foxglove.dart';
import 'package:logging/logging.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await PlayerPlatform.instance.initialize();

  Logger.root.level = Level.ALL;
  Logger.root.onRecord.listen((record) {
    // ignore: avoid_print
    print(
        '[${record.loggerName}] ${record.level.name}: ${record.time}: ${record.message}');
  });
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

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
    await PlayerPlatform.instance.configureLogging(const PlatformLogConfig(
        enableConsoleLogging: true,
        consoleLogLevel: PlatformLogLevel.trace,
        fileLogPath: 'platform_log.txt',
        fileLogLevel: PlatformLogLevel.trace));

    final player = await PlayerPlatform.instance
        .createPlayer(environmentArgs: ['--no-osd']);
    _player = player;

    if (mounted) {
      setState(() {});
    }

    final m = Media.file(File(r'some_video.mp4'));
    await player.setLoopMode(LoopMode.loop);
    await player.open(m);
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
