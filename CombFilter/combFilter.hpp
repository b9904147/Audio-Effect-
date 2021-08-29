#include <effect.h>

using namespace ape;
// max amount of samples we delay with
#define size 200
GlobalData(CombFilter, "");

class CombFilter : public Effect
{
public:
	Param<float> samples{ "Sample Delay", Range(0, 1) };
	
	Param<float> mix_cfg {"Mix",Range(0,1)};
	
	float buffer[size];
	
	unsigned ptr;
	

private:
	
	
		
	void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames) override
	{
		const auto shared = sharedChannels();
		unsigned sample=(unsigned)((1-samples)*size);
		float bsleft;
		float sleft;
		float mix = mix_cfg;
		float scale = (1 - mix) * 0.5 + 0.5;

		for (std::size_t c = 0; c < shared; ++c)
		{
			for (std::size_t n = 0; n < frames; ++n){
				// store input samples in our circular buffer				
				sleft = buffer[ptr] = inputs[c][n];
				// retrieve a buffer sample
				bsleft = buffer[(ptr + sample) % size] ;
				outputs[c][n] = (bsleft + sleft * mix) * scale;
				ptr++;
				if(ptr>(size-1))ptr=0;
			}
		}

		clear(outputs, shared);
	}
};