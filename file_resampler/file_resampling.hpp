#include <generator.h>

using namespace ape;

GlobalData(FileResampling, "");

class FileResampling : public Generator
{
public:

	enum class File
	{
		Sine,
		Sine16kHz,
		Square,
		Sweep
	};

	Param<File> fileParam{ "File",{"Sine","Sine16kHz","Suqare","Sweep"}};

	enum class Interpolation
	{
		Nearest,
		Linear,
		Hermite,
		Lagrange5,
		Lagrange10,
		Lagrange20,
		Sinc,
		Lanczos
	};

	static constexpr auto names = {
		"Nearest",
		"Linear",
		"Hermite",
		"Lagrange^5",
		"Lagrange^10",
		"Lagrange^20",
		"Sinc",
		"Lanczos"
	};

	Param<Interpolation> iLeft{ "Left", names }, iRight{ "Right", names };
	Param<int> window{"Kernel size", Range(1,128)};
	Param<float> volume{"Volume"};
	Param<float> speed{"Speed",Range(0.001,10,Range::Exp)};

	FileResampling()
	{
		volume = 0.1; //constructor
		speed = 1;
	}
private:
	
	std::vector<AudioFile> files {
		"perfect.wav",
		"16khz320hz.wav",
		"square.wav",
		"chirp.wav"
	};

	enum H
	{
		xm2,
		xm1,
		x0,
		x1,
		x2,
		x3,
	};

	uint64_t counter = 0;
	fpoint position = 0;
	fpoint history[7] {};	
	void process(umatrix<float> buffer, size_t frames) override
	{
		assert(buffer.channels()==2);
		if(files.empty()){
			abort("no files to play");
		}
		auto& file = files[(int)(File)fileParam];
		if(!file)
			abort("selected file not valid");
		const auto ratio = (fpoint)file.sampleRate()/config().sampleRate;
		const int wsize = window;
		circular_signal<const float> signal = file[0];
		auto interpolation = [&] (Interpolation interpolation, fpoint frac)
		{
			switch(interpolation)
			{
			case Interpolation::Nearest: 	return static_cast<fpoint>(signal((int)position));
			case Interpolation::Linear: 	return linear(frac, history[x0], history[x1]);
			case Interpolation::Hermite: 	return hermite4(frac, history[xm1], history[x0], history[x1], history[x2]);
			case Interpolation::Lagrange5: 	return lagrange5(frac, history[xm2], history[xm1], history[x0], history[x1], history[x2]);
			case Interpolation::Lagrange10: return lagrange<fpoint, 10>(signal, position);
			case Interpolation::Lagrange20: return lagrange<fpoint, 20>(signal, position);
			case Interpolation::Sinc: 		return sincFilter<fpoint>(signal, position, wsize);
			case Interpolation::Lanczos: 	return lanczosFilter<fpoint>(signal, position, wsize);
			}	
		};
		const Interpolation lPol = iLeft , rPol = iRight;
		const auto shared = sharedChannels();

		for (std::size_t n = 0; n < frames; ++n)
		{
			const auto x = static_cast<long long>(position);
			const auto frac = position - x;
			const auto offset = -2;
			for(size_t h =0;h<7;++h)
				history[h] =signal(n+h+offset);
			buffer[0][n] = interpolation(lPol,frac)*volume[n];
			buffer[1][n] = interpolation(rPol,frac)*volume[n];
			position += ratio * speed[n];
			while(position >= file.samples())
				position -= file.samples();
		}
	}
};