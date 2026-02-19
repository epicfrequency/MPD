// Copyright The Music Player Daemon Project

#include "PipeOutputPlugin.hxx"
#include "../OutputAPI.hxx"
#include "lib/fmt/SystemError.hxx"

#include <string>
#include <stdexcept>

#include <stdio.h>

class PipeOutput final : AudioOutput {
	const std::string cmd;
	FILE *fh;

	explicit PipeOutput(const ConfigBlock &block);

public:
	static AudioOutput *Create(EventLoop &,
				   const ConfigBlock &block) {
		return new PipeOutput(block);
	}

private:
	void Open(AudioFormat &audio_format) override;

	void Close() noexcept override {
		pclose(fh);
	}

	std::size_t Play(std::span<const std::byte> src) override;
};

PipeOutput::PipeOutput(const ConfigBlock &block)
	:AudioOutput(0),
	 cmd(block.GetBlockValue("command", ""))
{
	if (cmd.empty())
		throw std::runtime_error("No \"command\" parameter specified");
}

inline void
PipeOutput::Open(AudioFormat &audio_format)
{
	// audio_format is the final format MPD decided to feed into this pipe.
	// We inject it into the child process environment so your wrapper can
	// configure SDM pipeline parameters without watchers.

	const unsigned rate = audio_format.sample_rate;
	//const unsigned ch   = audio_format.channels;


    // Pass rate in argv so it shows up in htop
    std::string final_cmd = cmd + " --rate " + std::to_string(rate);

    // Optional debug:
    // fprintf(stderr, "pipe open: %s\n", final_cmd.c_str());

	fh = popen(final_cmd.c_str(), "w");
	if (fh == nullptr)
		throw FmtErrno("Error opening pipe {:?}", final_cmd);
}

std::size_t
PipeOutput::Play(std::span<const std::byte> src)
{
	size_t nbytes = fwrite(src.data(), 1, src.size(), fh);
	if (nbytes == 0)
		throw MakeErrno("Write error on pipe");

	return nbytes;
}

const struct AudioOutputPlugin pipe_output_plugin = {
	"pipe",
	nullptr,
	&PipeOutput::Create,
	nullptr,
};
