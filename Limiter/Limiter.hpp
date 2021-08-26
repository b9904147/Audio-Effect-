#include <effect.h>

using namespace ape;
GlobalData(Template, "");

class Template : public Effect
{
public:
	Param<float> threshold { "Threshold", Range(0, 1) };
	Param<float> release_time   {"Release time", Range(0, 1) };
	float env_set = 1;
	

private:
		
	void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames) override
	{
		const auto shared = sharedChannels();
		const float thres = threshold;
		const float release = release_time;
		float sample;
		float gain;
		float env = env_set; 
		float sampleAbs;
		float decay = exp(-1.0/48000*release);

		for (std::size_t c = 0; c < shared; ++c)
		{
			for (std::size_t n = 0; n < frames; ++n){
				sample = inputs[c][n];
				gain = 1.0; //reset gain
				sampleAbs = fabs(sample);
				if(sampleAbs > thres){
					if(sampleAbs>env)
					env = sampleAbs;
				}
				if(env > threshold)
					gain = thres/env;
				if(sampleAbs < threshold)
					env*=decay;
				outputs[c][n] =gain*sample; 
			}
		}
		env_set = env;
		clear(outputs, shared);
	}
};