import 'dart:io';

import 'package:flutter/material.dart';
import 'dart:async';

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
  Player? _player;

  @override
  void initState() {
    super.initState();
    initPlatformState();
  }

  // Platform messages are asynchronous, so we initialize in an async method.
  Future<void> initPlatformState() async {
    final p = PlayerPlatform.instance;

    //final envId = await p.createEnvironment(args: ['ABC', 'DEF']);
    //final player = await p.createPlayer(environmentId: envId);

    _player = await p.createPlayer(environmentArgs: [
      '--no-osd',
      '--file-caching=10000',
      '--high-priority',
      '--no-audio'
    ]);
    final player = _player!;

    setState(() {});

    player.generalStream.listen((event) {
      print("VOLUME CHANGED ${event.volume}");
    });

    player.playbackStream.listen((pb) {
      print("Status ${pb.playbackState}");
    });

    player.currentMediaStream.listen((event) {
      int x = 5;
    });

    //await player?.dispose();

    final m = Media.file(File(r'C:\Users\10261369\Videos\a.mp4'));

    //await p.disposeEnvironment(envId);

    final m2 = Media.file(File(r'C:\Users\10261369\Videos\b.mp4'));

    //await player?.open(m);

    final pl = Playlist(medias: [m, m2], playlistMode: PlaylistMode.loop);

    await player.open(m);
    await player.setPlaylistMode(PlaylistMode.loop);

    // await player.setVolume(0.75);

    //await player.seekTime(Duration(seconds: 5));

    //await player.play();

    await Future.delayed(Duration(seconds: 15));
    //await player.stop();

    await player.open(m2);

    //await Future.delayed(Duration(seconds: 5));

    //await player.dispose();

    //await player.play();

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
            child:
                _player == null ? const SizedBox() : Video(player: _player!)),
      ),
    );
  }
}
