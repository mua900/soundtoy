class AudioProcessor extends AudioWorkletProcessor {
    constructor(options) {
        super();
    }

    process(inputs, outputs, parameters) {
        const out = outputs[0];
        out.foreach((channel) => {
            for (let i = 0; i < channel.length; i++)
            {
                channel[i] = Math.random() * 2 - 1;
            }
        });

        return true;
    }
}

registerProcessor("audio_processor", AudioProcessor);
