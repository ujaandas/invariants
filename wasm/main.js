import createModule from "./invariants_wasm.js";

const source = document.querySelector("#source");

const tokensOutput = document.querySelector("#tokens-output");
const astOutput = document.querySelector("#ast-output");

const status = document.querySelector("#status");
const runButton = document.querySelector("#run");

const tabs = document.querySelectorAll(".tab");
const panels = document.querySelectorAll(".tab-panel");

let module = null;
let moduleLoadError = null;

function setActiveTab(name) {
    tabs.forEach((tab) => {
        tab.classList.toggle(
            "active",
            tab.dataset.tab === name
        );
    });

    panels.forEach((panel) => {
        panel.classList.toggle(
            "active",
            panel.id === `${name}-tab`
        );
    });
}

tabs.forEach((tab) => {
    tab.addEventListener("click", () => {
        setActiveTab(tab.dataset.tab);
    });
});

function pretty(json) {
    try {
        return JSON.stringify(JSON.parse(json), null, 2);
    } catch (err) {
        throw new Error(`Invalid JSON output: ${err.message ?? err}`);
    }
}

function treeText(json) {
    try {
        return JSON.parse(json).out;
    } catch (err) {
        throw new Error(`Invalid JSON output: ${err.message ?? err}`);
    }
}

function run() {
    if (!module) {
        status.textContent = `Error: ${moduleLoadError?.message ?? moduleLoadError ?? "wasm module failed to load."}`;
        return;
    }

    try {
        const tokens = module.tokenize(source.value);
        const ast = module.parse(source.value);

        tokensOutput.textContent = pretty(tokens);
        astOutput.textContent = treeText(ast);

        status.textContent = "Success.";
    } catch (err) {
        status.textContent = `Error: ${err.message ?? err}`;
    }
}

runButton.addEventListener("click", run);

try {
    module = await createModule();
    status.textContent = "Ready.";
} catch (err) {
    moduleLoadError = err;
    status.textContent = `Error: ${err.message ?? err}`;
}
