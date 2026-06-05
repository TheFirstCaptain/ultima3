import AVFoundation
import Ultima3Core

final class ShellAudioAdapter {
    private var queue = u3_audio_queue()
    private var soundPlayers: [AVAudioPlayer] = []
    private var musicPlayer: AVAudioPlayer?
    private var soundVolume: Float = 1.0
    private var musicVolume: Float = 1.0

    init() {
        u3_audio_queue_init(&queue)
    }

    func submitSound(_ sound: Int32) -> String {
        guard u3_audio_queue_push_sound(&queue, UInt16(sound), 1) != 0 else {
            return "Audio queue overflow"
        }
        return consumeNextDescription()
    }

    func submitMusic(_ music: Int32) -> String {
        guard u3_audio_queue_push_music(&queue, UInt16(music)) != 0 else {
            return "Audio queue overflow"
        }
        return consumeNextDescription()
    }

    func stopMusic() -> String {
        guard u3_audio_queue_push_stop_music(&queue) != 0 else {
            return "Audio queue overflow"
        }
        return consumeNextDescription()
    }

    private func consumeNextDescription() -> String {
        var event = u3_audio_event()

        guard u3_audio_queue_pop(&queue, &event) != 0 else {
            return "Audio queue empty"
        }

        switch Int32(event.kind) {
        case U3_AUDIO_EVENT_PLAY_SOUND:
            return playSound(event.resource_id)
        case U3_AUDIO_EVENT_PLAY_MUSIC:
            return playMusic(event.resource_id)
        case U3_AUDIO_EVENT_STOP_MUSIC:
            musicPlayer?.stop()
            musicPlayer = nil
            return "Audio Stop Music"
        case U3_AUDIO_EVENT_SET_SOUND_VOLUME:
            soundVolume = Float(event.volume_percent) / 100
            return "Audio Sound Volume \(event.volume_percent)"
        case U3_AUDIO_EVENT_SET_MUSIC_VOLUME:
            musicVolume = Float(event.volume_percent) / 100
            musicPlayer?.volume = musicVolume
            return "Audio Music Volume \(event.volume_percent)"
        default:
            return "Audio Unknown"
        }
    }

    private func playSound(_ resourceID: UInt16) -> String {
        let name = soundName(resourceID)
        guard let url = resourceURL(directory: "Resources/Sounds", filename: "\(name).wav") else {
            return "Audio Missing \(name)"
        }

        do {
            let player = try AVAudioPlayer(contentsOf: url)
            player.volume = soundVolume
            player.prepareToPlay()
            player.play()
            soundPlayers.append(player)
            soundPlayers.removeAll { !$0.isPlaying }
            return "Audio Sound \(name)"
        } catch {
            return "Audio Failed \(name)"
        }
    }

    private func playMusic(_ resourceID: UInt16) -> String {
        let name = musicName(resourceID)
        guard let url = resourceURL(directory: "Resources/Music", filename: "\(name).mov") else {
            return "Audio Missing \(name)"
        }

        do {
            let player = try AVAudioPlayer(contentsOf: url)
            player.volume = musicVolume
            player.numberOfLoops = -1
            player.prepareToPlay()
            player.play()
            musicPlayer = player
            return "Audio Music \(name)"
        } catch {
            return "Audio Failed \(name)"
        }
    }

    private func resourceURL(directory: String, filename: String) -> URL? {
        let fileManager = FileManager.default
        let currentDirectory = URL(fileURLWithPath: fileManager.currentDirectoryPath, isDirectory: true)
        let candidates = [
            currentDirectory.appendingPathComponent(directory).appendingPathComponent(filename),
            currentDirectory.deletingLastPathComponent().appendingPathComponent(directory).appendingPathComponent(filename),
            Bundle.main.resourceURL?.appendingPathComponent(directory).appendingPathComponent(filename)
        ].compactMap { $0 }

        return candidates.first { fileManager.fileExists(atPath: $0.path) }
    }

    private func soundName(_ resourceID: UInt16) -> String {
        switch Int32(resourceID) {
        case U3_AUDIO_SOUND_STEP:
            return "Step"
        case U3_AUDIO_SOUND_ERROR1:
            fallthrough
        default:
            return "Error1"
        }
    }

    private func musicName(_ resourceID: UInt16) -> String {
        switch Int32(resourceID) {
        case U3_AUDIO_MUSIC_SONG_1:
            fallthrough
        default:
            return "Song_1"
        }
    }
}
