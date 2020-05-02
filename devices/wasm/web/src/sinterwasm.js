import sinterwasm from "../../build/sinterwasm.js";
import sinterwasmModule from "../../build/sinterwasm.wasm";

// Adapted from https://gist.github.com/surma/b2705b6cca29357ebea1c9e6e15684cc

const future = (props) => new Promise((accept, reject) => {
  const module = sinterwasm({
    locateFile(path) {
      if (path.endsWith(".wasm")) {
        return sinterwasmModule;
      }
      return path;
    },
    onRuntimeInitialized() {
      const alloc_heap = module.cwrap("siwasm_alloc_heap", null, ["number"]);
      const alloc = module.cwrap("siwasm_alloc", "number", ["number"]);
      const free = module.cwrap("siwasm_free", null, ["number"]);
      const run = module.cwrap("siwasm_run", null, ["number", "number"]);
      accept({
        module,
        alloc_heap,
        alloc,
        free,
        run,
      });
    },
    ...props
  });
});

export default future;
