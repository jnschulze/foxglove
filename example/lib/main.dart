import 'dart:io';

import 'package:flutter/material.dart';
import 'dart:async';

import 'package:flutter/services.dart';
import 'package:foxglove/foxglove.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  String _platformVersion = 'Unknown';

  @override
  void initState() {
    super.initState();
    initPlatformState();
  }

  // Platform messages are asynchronous, so we initialize in an async method.
  Future<void> initPlatformState() async {
    final p = PlayerPlatform();

    //final envId = await p.createEnvironment(args: ['ABC', 'DEF']);
    //final player = await p.createPlayer(environmentId: envId);

    final player = await p.createPlayer();
    //await player?.dispose();

    final m = Media.file(
        File(r'C:\Users\10261369\Videos\bbb_sunflower_1080p_30fps_normal.mp4'));

    //await p.disposeEnvironment(envId);

    await player?.open(m);

    await player?.play();

    int x = 5;
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: const Text('Plugin example app'),
        ),
        body: Center(
          child: Text('Running on: $_platformVersion\n'),
        ),
      ),
    );
  }
}
