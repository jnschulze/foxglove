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

    final m = Media.file(File(r'C:\Users\10261369\Videos\a.mp4'));

    //await p.disposeEnvironment(envId);

    final m2 = Media.file(File(r'C:\Users\10261369\Videos\b.mp4'));

    //await player?.open(m);

    //final pl = Playlist(medias: [m], playlistMode: PlaylistMode.repeat);

    await player?.open(m);
    await player?.setPlaylistMode(PlaylistMode.loop);

    await player?.play();

    await Future.delayed(Duration(seconds: 10));

    //await player?.open(m2);

    //await player?.stop();

    //await player?.dispose();

    //int x = 5;
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
