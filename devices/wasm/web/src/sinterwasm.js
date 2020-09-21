import sinterwasm from "../../build/sinterwasm.js";
import sinterwasmModule from "../../build/sinterwasm.wasm";

// Adapted from https://gist.github.com/surma/b2705b6cca29357ebea1c9e6e15684cc

const future = async (props) => {
  const module = await sinterwasm({
    locateFile(path) {
      if (path.endsWith(".wasm")) {
        return sinterwasmModule;
      }
      return path;
    },
    ...props,
  });

  if (!module.cwrap) {
    console.error("module has no cwrap", module);
    throw new Error("module has no cwrap");
  }
  const alloc_heap = module.cwrap("siwasm_alloc_heap", null, ["number"]);
  const alloc = module.cwrap("siwasm_alloc", "number", ["number"]);
  const free = module.cwrap("siwasm_free", null, ["number"]);
  const run = module.cwrap("siwasm_run", null, ["number", "number"]);
  return {
    module,
    alloc_heap,
    alloc,
    free,
    run,
  };
};

export default future;
