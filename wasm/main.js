import createModule from "./invariants_wasm.js";

const source = document.querySelector("#source");

const tokensOutput = document.querySelector("#tokens-output");
const astOutput = document.querySelector("#ast-output");

const status = document.querySelector("#status");
const runButton = document.querySelector("#run");

const tabs = document.querySelectorAll(".tab");
const panels = document.querySelectorAll(".tab-panel");

const module = await createModule();

status.textContent = "Ready.";

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
    return JSON.stringify(JSON.parse(json), null, 2);
}

function run() {
    try {
        const tokens = module.tokenize(source.value);
        const ast = module.parse(source.value);

        tokensOutput.textContent = pretty(tokens);
        astOutput.textContent = pretty(ast);

        status.textContent = "Success.";
    } catch (err) {
        status.textContent = `Error: ${err}`;
    }
}

runButton.addEventListener("click", run);