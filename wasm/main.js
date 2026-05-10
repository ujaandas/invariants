import createModule from "./invariants_wasm.js";

const source = document.querySelector("#source");
const output = document.querySelector("#output");
const status = document.querySelector("#status");
const runButton = document.querySelector("#run");

const module = await createModule();

status.textContent = "Ready.";

function tokenize() {
    const result = module.tokenize(source.value);

    output.textContent = JSON.stringify(
        JSON.parse(result),
        null,
        2
    );
}

runButton.addEventListener("click", tokenize);