export class StAudio {
    constructor(sample_rate, channel_count, sampler_left, sampler_right) {
        this.audio_context = null;
        this.sample_rate = sample_rate;
        this.channel_count = channel_count;
        this.sampler_left = sampler_left;
        this.sampler_right = sampler_right;
        this.paused = true;
        this.workletNode = null;
    }

    async initialize() {
        this.audio_context = new AudioContext();
        this.audio_context.suspend();
        this.gainNode = new GainNode(this.audio_context);
        this.gainNode.gain.value = 0.0;

        await this.audio_context.audioWorklet.addModule("audio_processor.js");
        this.workletNode = new AudioWorkletNode(this.audio_context, "audio_processor");

        this.workletNode.connect(this.gainNode).connect(this.audio_context.destination);
        await this.audio_context.suspend();
    }

    getVolume() {
        if (this.gainNode) {
            return this.gainNode.gain.value;
        }
        else {
            return 0.0;
        }
    }

    setVolume(volume) {
        if (this.gainNode) {
            this.gainNode.gain.value = volume;
        }
    }

    async pause() {
        await this.audio_context.suspend();
        this.paused = true;
    }

    async unpause() {
        await this.audio_context.resume();
        this.paused = false;
    }

    toggle_pause() {
        if (this.paused) {
            this.unpause();
        }
        else {
            this.pause();
        }
    }
}
