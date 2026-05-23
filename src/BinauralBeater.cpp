#include "plugin.hpp"

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
    V_OCT_IN_INPUT,
    PARAMS_LEN
  };
  enum InputId {
    BF_CV_IN_INPUT,
    AMR_CV_IN_INPUT,
    AMD_CV_IN_INPUT,
    FMRATE_CV_IN_INPUT,
    FMRANGE_CV_IN_INPUT,
    INPUTS_LEN
  };
  enum OutputId { L_OUT_OUTPUT, R_OUT_OUTPUT, OUTPUTS_LEN };
  enum LightId { BF_LIGHT_LIGHT, LIGHTS_LEN };

  BinauralBeater() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(BF_CV_KNOB_PARAM, 0.f, 1.f, 0.f, "Beat Frequency CV", "Hz"); // Beat Frequency CV  0-1, 0
    configParam(BF_KNOB1_PARAM, 0.f, 100.f, 4.f, "Beat Frequency (differential)", "Hz"); // Beat Frequency 0-100, 4
    configParam(CF_KNOB1_PARAM, 1.f, 20000.f, 261.63f, "Carrier Frequency (base)", "Hz"); // Carrier Frequency 1-20kHz, 261.63
    configParam(AM_RATE_KNOB_PARAM, 0.f, 100.f, 0.f, "Amplitude Modulation Rate", "Hz"); // AM Rate 0-100Hz, 0
    configParam(AM_DEPTH_KNOB_PARAM, 0.f, 1.f, 0.f, "Amplitude Modulation Depth"); // AM Depth 0-1, 0
    configParam(FM_RATE_KNOB_PARAM, 0.f, 100.f, 0.f, "Frequency Modulation Rate"); // FM Rate 0-100Hz, 0
    configParam(FM_RANGE_KNOB_PARAM, 0.f, 100.f, 0.f, "Frequency Modulation Range"); // FM Range 0-100Hz, 0
    configParam(AM_RATE_CV_KNOB_PARAM, 0.f, 1.f, 0.f, "Amplitude Modulation Rate CV"); // AM Rate CV 0-1, 0
    configParam(AM_DEPTH_CV_KNOB_PARAM, 0.f, 1.f, 0.f, "Amplitude Modulation Depth CV"); // AM Depth CV 0-1, 0
    configParam(FM_RATE_CV_KNOB_PARAM, 0.f, 1.f, 0.f, "Frequency Modulation Rate CV"); // FM Rate CV 0-1, 0
    configParam(FM_RANGE_CV_KNOB_PARAM, 0.f, 1.f, 0.f, "Frequency Modulation Range CV"); // FM Range CV 0-1, 0
    configSwitch(LR_SWITCH_PARAM, 0.f, 1.f, 0.f, "Left/Right High Frequency Switch", {"Left", "Right"}); // Left/Right High Frequency Switch 0-1, 0
    configParam(V_OCT_IN_INPUT, -5.f, 5.f, 0.f, "Pitch CV", "V/Octave"); // V/Oct Pitch CV -5V to 5V, 0V
    configInput(BF_CV_IN_INPUT, "Beat Frequency Modulation CV Input");
    configInput(AMR_CV_IN_INPUT, "Amplitude Modulation CV Input");
    configInput(AMD_CV_IN_INPUT, "Amplitude Modulation Depth CV Input");
    configInput(FMRATE_CV_IN_INPUT, "FM Rate CV Input");
    configInput(FMRANGE_CV_IN_INPUT, "FM Range CV Input");
    configOutput(L_OUT_OUTPUT, "Left Output");
    configOutput(R_OUT_OUTPUT, "Right Output");
  }

  void process(const ProcessArgs &args) override {}
};

struct BinauralBeaterWidget : ModuleWidget {
  BinauralBeaterWidget(BinauralBeater *module) {
    setModule(module);
    setPanel(
        createPanel(asset::plugin(pluginInstance, "res/BinauralBeater.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(
        createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(
        Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(Vec(
        box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(326.188, 369.195)), module,
        BinauralBeater::BF_CV_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(601.348, 369.195)), module, BinauralBeater::BF_KNOB1_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(1027.846, 369.195)), module, BinauralBeater::CF_KNOB1_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(174.459, 758.858)), module,
        BinauralBeater::AM_RATE_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(462.219, 758.858)), module,
        BinauralBeater::AM_DEPTH_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(744.161, 758.858)), module,
        BinauralBeater::FM_RATE_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(1029.129, 758.858)), module,
        BinauralBeater::FM_RANGE_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(174.655, 999.082)), module,
        BinauralBeater::AM_RATE_CV_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(462.415, 999.082)), module,
        BinauralBeater::AM_DEPTH_CV_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(745.023, 999.082)), module,
        BinauralBeater::FM_RATE_CV_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(1028.041, 999.082)), module,
        BinauralBeater::FM_RANGE_CV_KNOB_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(176.789, 1393.511)), module,
        BinauralBeater::LR_SWITCH_PARAM));

    addInput(createInputCentered<PJ301MPort>(
        mm2px(Vec(110.03, 140.588)), module, BinauralBeater::V_OCT_IN_INPUT));
    addInput(createInputCentered<PJ301MPort>(
        mm2px(Vec(110.03, 367.574)), module, BinauralBeater::BF_CV_IN_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(176.789, 1213.664)),
                                             module,
                                             BinauralBeater::AMR_CV_IN_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(460.281, 1213.664)),
                                             module,
                                             BinauralBeater::AMD_CV_IN_INPUT));
    addInput(
        createInputCentered<PJ301MPort>(mm2px(Vec(745.023, 1213.664)), module,
                                        BinauralBeater::FMRATE_CV_IN_INPUT));
    addInput(
        createInputCentered<PJ301MPort>(mm2px(Vec(1028.041, 1213.664)), module,
                                        BinauralBeater::FMRANGE_CV_IN_INPUT));

    addOutput(createOutputCentered<PJ301MPort>(
        mm2px(Vec(747.157, 1391.444)), module, BinauralBeater::L_OUT_OUTPUT));
    addOutput(createOutputCentered<PJ301MPort>(
        mm2px(Vec(1028.041, 1391.444)), module, BinauralBeater::R_OUT_OUTPUT));

    addChild(createLightCentered<MediumLight<RedLight>>(
        mm2px(Vec(732.243, 369.195)), module, BinauralBeater::BF_LIGHT_LIGHT));

    // Input Configuration / tooltips

  }
};

Model *modelBinauralBeater =
    createModel<BinauralBeater, BinauralBeaterWidget>("BinauralBeater");
