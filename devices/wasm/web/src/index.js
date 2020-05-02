import "./index.css";

import { useState, useEffect, useCallback } from "preact/hooks";

import createContext from "js-slang/dist/createContext";
import { parse } from "js-slang/dist/parser/parser";
import { compileToIns } from "js-slang/dist/vm/svml-compiler";
import { assemble } from "js-slang/dist/vm/svml-assembler";
import { stringifyProgram } from "js-slang/dist/vm/util";
import loadSinterwasm from "./sinterwasm";
import AceEditor from "react-ace";
import { saveAs } from 'file-saver';

import "ace-builds/src-noconflict/mode-javascript";
import "ace-builds/src-noconflict/theme-monokai";

let stdoutPre = document.createElement("pre");
stdoutPre.id = "stdout";
let stderrPre = document.createElement("pre");
stderrPre.id = "stderr";
let scrollOutput = true;
function onStdout(str) {
  let e = document.createTextNode(str);
  stdoutPre.appendChild(e);
  if (scrollOutput) {
    const p = stdoutPre.parentElement;
    p.scrollTop = p.scrollHeight;
  }
}
function onStderr(str) {
  let e = document.createTextNode(str);
  stderrPre.appendChild(e);
  if (scrollOutput) {
    const p = stderrPre.parentElement;
    p.scrollTop = p.scrollHeight;
  }
}
function clearOutput() {
  stdoutPre.textContent = "";
  stderrPre.textContent = "";
}

function resizeHeap() {
  const newSizeStr = prompt("Enter new heap size, in bytes");
  if (!newSizeStr) {
    return;
  }
  const newSize = parseInt(newSizeStr);
  if (newSize < 0 || isNaN(newSize) || !isFinite(newSize)) {
    onStderr(`Invalid heap size!\n`);
    return;
  }
  sinterwasm.alloc_heap(newSize);
  onStdout(`Resized heap to ${newSize} bytes.\n`);
}

let editorCode = "";
let sinterwasm = null;

function formatError(error) {
  const sev = error.severity ? `[${error.severity}] ` : "";
  const loc = error.location ? `(${error.location.start.line}:${error.location.start.column}) ` : "";
  const exp = error.explain ? error.explain() : error.message;
  return `${sev}${loc}${exp}\n`;
}

let clearBeforeRun = true;
function compile() {
  if (clearBeforeRun) {
    clearOutput();
  }

  const context = createContext(3, "default");
  const ast = parse(editorCode, context);

  let numWarnings = 0;
  let numErrors = 0;
  for (const error of context.errors) {
    onStderr(formatError(error));

    switch (error.severity) {
      case "Warning":
        ++numWarnings;
        break;
      case "Error":
        ++numErrors;
        break;
    }
  }

  if (numWarnings > 0 || numErrors > 0) {
    onStderr(`${numWarnings} warning(s) and ${numErrors} error(s) produced.\n`);
  }

  if (typeof ast === "undefined") {
    return null;
  }

  let asm;
  try {
    asm = compileToIns(ast);
  } catch (e) {
    onStderr(formatError(e));
    return null;
  }

  return asm;
}

function run() {
  let bin;
  try {
    bin = assemble(compile());
  } catch (e) {
    onStderr(formatError(e));
    return;
  }

  const emsMem = sinterwasm.alloc(bin.byteLength);
  if (!emsMem) {
    onStderr("Failed to allocate WebAssembly memory.\n");
    return;
  }

  sinterwasm.module.HEAPU8.set(bin, emsMem);
  sinterwasm.run(emsMem, bin.byteLength);
}

function toAsm() {
  const dump = stringifyProgram(compile());
  onStdout(dump + "\n");
}

function save() {
  let bin;
  try {
    bin = assemble(compile());
  } catch (e) {
    onStderr(formatError(e));
    return;
  }

  const blob = new Blob([bin], { type: "application/octet-stream" });
  saveAs(blob, "program.svm");
}

const sinterwasmFuture = loadSinterwasm({
  print: (str) => onStdout(str + "\n"),
  printErr: (str) => onStderr(str + "\n"),
});

sinterwasmFuture.then((module) => {
  module.alloc_heap(0x10000);
  sinterwasm = module;
});

export default function App() {
  const [loaded, setLoaded] = useState(false);
  const [follow, setFollow] = useState(scrollOutput);
  const [clear, setClear] = useState(clearBeforeRun);
  useEffect(() => {
    sinterwasmFuture.then(() => {
      setLoaded(true);
    });
  }, []);
  const stdoutRef = useCallback((stdoutCtr) => {
    if (stdoutCtr) {
      stdoutCtr.appendChild(stdoutPre);
    }
  }, []);
  const stderrRef = useCallback((stderrCtr) => {
    if (stderrCtr) {
      stderrCtr.appendChild(stderrPre);
    }
  }, []);
  function toggleFollow() {
    const newFollow = !follow;
    setFollow(newFollow);
    scrollOutput = newFollow;
  }
  function toggleClear() {
    const newClear = !clear;
    setClear(newClear);
    clearBeforeRun = newClear;
  }

  return (
    <div id="app">
      <div id="editor">
        <AceEditor
          mode="javascript"
          theme="monokai"
          onChange={(code) => {
            editorCode = code;
          }}
          height="100%"
          width="100%"
          fontSize={16}
        />
      </div>
      <div id="control">
        {loaded ? (
          <div id="buttons">
            <button onClick={run}>Run</button>
            <button onClick={toAsm}>Assembly</button>
            <button onClick={save}>Save SVM</button>
            <button onClick={clearOutput}>Clear output</button>
            <button onClick={resizeHeap}>Resize heap</button>
            <button onClick={toggleClear}>{clear ? "☑" : "☐"} Autoclear</button>
            <button onClick={toggleFollow}>{follow ? "☑" : "☐"} Autoscroll</button>
          </div>
        ) : (
          <h2>Loading...</h2>
        )}
        <div class="output-section">
          <h2>Standard output</h2>
          <div class="output-container" ref={stdoutRef}></div>
        </div>
        <div class="output-section">
          <h2>Standard error</h2>
          <div class="output-container" ref={stderrRef}></div>
        </div>
      </div>
    </div>
  );
}
