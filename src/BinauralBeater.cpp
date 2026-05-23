#include "plugin.hpp"

#include <cmath>

namespace {

constexpr float kDefaultSampleRate = 44100.f;
constexpr float kMinFrequencyHz = 1.f;
constexpr float kMaxFrequencyHz = 20000.f;
constexpr float kMaxParamRateHz = 100.f;
constexpr float kAudioLevelVolts = 5.f;
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr int kSchemaVersion = 1;

struct EffectiveControls {
	float beatHz = 4.f;
	float carrierHz = 261.63f;
	float amRateHz = 0.f;
	float amDepth = 0.f;
	float fmRateHz = 0.f;
	float fmRangeHz = 0.f;
	bool leftHigh = false;
};

struct PersistentState {
	int schemaVersion = kSchemaVersion;
};

float wrapPhase(float phase) {
	phase -= std::floor(phase);
	if (phase < 0.f) {
		phase += 1.f;
	}
	return phase;
}

float clampFrequency(float frequencyHz) {
	return clamp(frequencyHz, kMinFrequencyHz, kMaxFrequencyHz);
}

float applyScaledCv(float knobValue, float cvVoltage, float cvAmount, float minValue, float maxValue) {
	const float span = maxValue - minValue;
	const float normalizedCv = clamp(cvVoltage / 5.f, -1.f, 1.f);
	return clamp(knobValue + normalizedCv * cvAmount * span, minValue, maxValue);
}

} // namespace

struct BinauralBeater : Module {
	enum ParamId {
		BF_CV_KNOB_PARAM,
		BF_KNOB1_PARAM,
		CF_KNOB1_PARAM,
		AM_RATE_KNOB_PARAM,
		AM_DEPTH_KNOB_PARAM,
		FM_RATE_KNOB_PARAM,
		FM_RANGE_KNOB_PARAM,
		AM_RATE_CV_KNOB_PARAM,
		AM_DEPTH_CV_KNOB_PARAM,
		FM_RATE_CV_KNOB_PARAM,
		FM_RANGE_CV_KNOB_PARAM,
		LR_SWITCH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		V_OCT_IN_INPUT,
		BF_CV_IN_INPUT,
		AMR_CV_IN_INPUT,
		AMD_CV_IN_INPUT,
		FMRATE_CV_IN_INPUT,
		FMRANGE_CV_IN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		L_OUT_OUTPUT,
		R_OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		BF_LIGHT_LIGHT,
		LIGHTS_LEN
	};

	float sampleRate = kDefaultSampleRate;
	float sampleTime = 1.f / kDefaultSampleRate;
	float smoothingFactor = 1.f;
	float leftPhase = 0.f;
	float rightPhase = 0.f;
	float amPhase = 0.f;
	float fmPhase = 0.f;
	bool controlsPrimed = false;
	EffectiveControls smoothedControls;
	PersistentState persistentState;

	BinauralBeater() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(BF_CV_KNOB_PARAM, 0.f, 1.f, 0.f, "Beat frequency CV amount");
		configParam(BF_KNOB1_PARAM, 0.f, 100.f, 4.f, "Beat frequency", " Hz");
		configParam(CF_KNOB1_PARAM, 1.f, 20000.f, 261.63f, "Carrier frequency", " Hz");
		configParam(AM_RATE_KNOB_PARAM, 0.f, 100.f, 0.f, "Amplitude modulation rate", " Hz");
		configParam(AM_DEPTH_KNOB_PARAM, 0.f, 1.f, 0.f, "Amplitude modulation depth");
		configParam(FM_RATE_KNOB_PARAM, 0.f, 100.f, 0.f, "Frequency modulation rate", " Hz");
		configParam(FM_RANGE_KNOB_PARAM, 0.f, 100.f, 0.f, "Frequency modulation range", " Hz");
		configParam(AM_RATE_CV_KNOB_PARAM, 0.f, 1.f, 0.f, "Amplitude modulation rate CV amount");
		configParam(AM_DEPTH_CV_KNOB_PARAM, 0.f, 1.f, 0.f, "Amplitude modulation depth CV amount");
		configParam(FM_RATE_CV_KNOB_PARAM, 0.f, 1.f, 0.f, "Frequency modulation rate CV amount");
		configParam(FM_RANGE_CV_KNOB_PARAM, 0.f, 1.f, 0.f, "Frequency modulation range CV amount");
		configSwitch(LR_SWITCH_PARAM, 0.f, 1.f, 0.f, "High frequency output side", {"Left", "Right"});

		configInput(V_OCT_IN_INPUT, "Pitch CV");
		configInput(BF_CV_IN_INPUT, "Beat frequency CV");
		configInput(AMR_CV_IN_INPUT, "Amplitude modulation rate CV");
		configInput(AMD_CV_IN_INPUT, "Amplitude modulation depth CV");
		configInput(FMRATE_CV_IN_INPUT, "Frequency modulation rate CV");
		configInput(FMRANGE_CV_IN_INPUT, "Frequency modulation range CV");

		configOutput(L_OUT_OUTPUT, "Left output");
		configOutput(R_OUT_OUTPUT, "Right output");

		refreshSampleRate();
		resetRuntimeState();
	}

	void refreshSampleRate() {
		if (APP && APP->engine) {
			sampleRate = APP->engine->getSampleRate();
		}
		if (!(sampleRate > 0.f)) {
			sampleRate = kDefaultSampleRate;
		}
		sampleTime = 1.f / sampleRate;

		const float smoothingTimeSeconds = 0.01f;
		smoothingFactor = std::exp(-sampleTime / smoothingTimeSeconds);
	}

	void resetRuntimeState() {
		leftPhase = 0.f;
		rightPhase = 0.f;
		amPhase = 0.f;
		fmPhase = 0.f;
		controlsPrimed = false;
		smoothedControls = EffectiveControls();
	}

	EffectiveControls resolveTargets() {
		EffectiveControls targets;
		targets.beatHz = applyScaledCv(
		    params[BF_KNOB1_PARAM].getValue(),
		    inputs[BF_CV_IN_INPUT].getVoltage(),
		    params[BF_CV_KNOB_PARAM].getValue(),
		    0.f,
		    100.f);

		const float pitchVolts = inputs[V_OCT_IN_INPUT].getVoltage();
		const float baseCarrierHz = params[CF_KNOB1_PARAM].getValue() * std::pow(2.f, pitchVolts);
		targets.carrierHz = clampFrequency(baseCarrierHz);

		targets.amRateHz = applyScaledCv(
		    params[AM_RATE_KNOB_PARAM].getValue(),
		    inputs[AMR_CV_IN_INPUT].getVoltage(),
		    params[AM_RATE_CV_KNOB_PARAM].getValue(),
		    0.f,
		    kMaxParamRateHz);
		targets.amDepth = applyScaledCv(
		    params[AM_DEPTH_KNOB_PARAM].getValue(),
		    inputs[AMD_CV_IN_INPUT].getVoltage(),
		    params[AM_DEPTH_CV_KNOB_PARAM].getValue(),
		    0.f,
		    1.f);
		targets.fmRateHz = applyScaledCv(
		    params[FM_RATE_KNOB_PARAM].getValue(),
		    inputs[FMRATE_CV_IN_INPUT].getVoltage(),
		    params[FM_RATE_CV_KNOB_PARAM].getValue(),
		    0.f,
		    kMaxParamRateHz);
		targets.fmRangeHz = applyScaledCv(
		    params[FM_RANGE_KNOB_PARAM].getValue(),
		    inputs[FMRANGE_CV_IN_INPUT].getVoltage(),
		    params[FM_RANGE_CV_KNOB_PARAM].getValue(),
		    0.f,
		    100.f);
		targets.leftHigh = params[LR_SWITCH_PARAM].getValue() < 0.5f;
		return targets;
	}

	void primeSmoothedControls(const EffectiveControls& targets) {
		smoothedControls = targets;
		controlsPrimed = true;
	}

	void smoothControls(const EffectiveControls& targets) {
		const float blend = 1.f - smoothingFactor;
		smoothedControls.beatHz += (targets.beatHz - smoothedControls.beatHz) * blend;
		smoothedControls.carrierHz += (targets.carrierHz - smoothedControls.carrierHz) * blend;
		smoothedControls.amRateHz += (targets.amRateHz - smoothedControls.amRateHz) * blend;
		smoothedControls.amDepth += (targets.amDepth - smoothedControls.amDepth) * blend;
		smoothedControls.fmRateHz += (targets.fmRateHz - smoothedControls.fmRateHz) * blend;
		smoothedControls.fmRangeHz += (targets.fmRangeHz - smoothedControls.fmRangeHz) * blend;
		smoothedControls.leftHigh = targets.leftHigh;
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "schemaVersion", json_integer(persistentState.schemaVersion));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		persistentState = PersistentState();

		if (!rootJ) {
			return;
		}

		json_t* schemaVersionJ = json_object_get(rootJ, "schemaVersion");
		if (schemaVersionJ && json_is_integer(schemaVersionJ)) {
			persistentState.schemaVersion = clamp(static_cast<int>(json_integer_value(schemaVersionJ)), 1, kSchemaVersion);
		}
	}

	void onReset() override {
		resetRuntimeState();
	}

	void onSampleRateChange() override {
		refreshSampleRate();
	}

	void process(const ProcessArgs& args) override {
		const EffectiveControls targets = resolveTargets();
		if (!controlsPrimed) {
			primeSmoothedControls(targets);
		}
		smoothControls(targets);

		fmPhase = wrapPhase(fmPhase + smoothedControls.fmRateHz * sampleTime);
		amPhase = wrapPhase(amPhase + smoothedControls.amRateHz * sampleTime);

		const float fmOffsetHz = std::sin(kTwoPi * fmPhase) * smoothedControls.fmRangeHz;
		const float lowEarHz = clampFrequency(smoothedControls.carrierHz + fmOffsetHz);
		const float highEarHz = clampFrequency(lowEarHz + smoothedControls.beatHz);
		const float amGain = 1.f - smoothedControls.amDepth * 0.5f * (std::sin(kTwoPi * amPhase) + 1.f);

		const float leftFrequencyHz = smoothedControls.leftHigh ? highEarHz : lowEarHz;
		const float rightFrequencyHz = smoothedControls.leftHigh ? lowEarHz : highEarHz;

		leftPhase = wrapPhase(leftPhase + leftFrequencyHz * sampleTime);
		rightPhase = wrapPhase(rightPhase + rightFrequencyHz * sampleTime);

		const float leftOutput = std::sin(kTwoPi * leftPhase) * amGain * kAudioLevelVolts;
		const float rightOutput = std::sin(kTwoPi * rightPhase) * amGain * kAudioLevelVolts;

		outputs[L_OUT_OUTPUT].setVoltage(leftOutput);
		outputs[R_OUT_OUTPUT].setVoltage(rightOutput);

		const float lightLevel = clamp(
		    smoothedControls.beatHz / 100.f + smoothedControls.amDepth * 0.25f + smoothedControls.fmRangeHz / 100.f * 0.25f,
		    0.f,
		    1.f);
		lights[BF_LIGHT_LIGHT].setBrightnessSmooth(lightLevel, args.sampleTime);
	}
};

struct BinauralBeaterWidget : ModuleWidget {
	BinauralBeaterWidget(BinauralBeater* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/BinauralBeater.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(326.188, 369.195)), module, BinauralBeater::BF_CV_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(601.348, 369.195)), module, BinauralBeater::BF_KNOB1_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(1027.846, 369.195)), module, BinauralBeater::CF_KNOB1_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(174.459, 758.858)), module, BinauralBeater::AM_RATE_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(462.219, 758.858)), module, BinauralBeater::AM_DEPTH_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(744.161, 758.858)), module, BinauralBeater::FM_RATE_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(1029.129, 758.858)), module, BinauralBeater::FM_RANGE_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(174.655, 999.082)), module, BinauralBeater::AM_RATE_CV_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(462.415, 999.082)), module, BinauralBeater::AM_DEPTH_CV_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(745.023, 999.082)), module, BinauralBeater::FM_RATE_CV_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(1028.041, 999.082)), module, BinauralBeater::FM_RANGE_CV_KNOB_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(176.789, 1393.511)), module, BinauralBeater::LR_SWITCH_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(110.03, 140.588)), module, BinauralBeater::V_OCT_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(110.03, 367.574)), module, BinauralBeater::BF_CV_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(176.789, 1213.664)), module, BinauralBeater::AMR_CV_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(460.281, 1213.664)), module, BinauralBeater::AMD_CV_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(745.023, 1213.664)), module, BinauralBeater::FMRATE_CV_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(1028.041, 1213.664)), module, BinauralBeater::FMRANGE_CV_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(747.157, 1391.444)), module, BinauralBeater::L_OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(1028.041, 1391.444)), module, BinauralBeater::R_OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(732.243, 369.195)), module, BinauralBeater::BF_LIGHT_LIGHT));
	}
};

Model* modelBinauralBeater = createModel<BinauralBeater, BinauralBeaterWidget>("BinauralBeater");
