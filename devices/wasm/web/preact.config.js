export default (config, env, helpers) => {
  config.module.rules.push({
    test: /\.wasm$/,
    type: "javascript/auto",
    loader: "file-loader",
  });
  config.node.fs = "empty";
  config.node.child_process = "empty";
  config.node.Buffer = true;
  config.node.process = true;

  let babelConfig = helpers.getLoadersByName(config, 'babel-loader')[0].rule.options;
  let babelPresetEnvPresets = babelConfig.presets.find(([path]) => path.includes("@babel/preset-env"))[1];
  babelPresetEnvPresets.targets = "Firefox ESR";
  babelPresetEnvPresets.loose = false;
  // console.log(babelPresetEnvPresets);
};
