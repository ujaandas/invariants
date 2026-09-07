import glob
import os

from huggingface_hub import snapshot_download
from huggingface_hub.utils import LocalEntryNotFoundError
from llama_cpp import Llama, LogitsProcessor, LogitsProcessorList

from invariants.State import DecodeState


def resolve_cached_model_path(repo_id: str, filename: str) -> str | None:
    try:
        snapshot_dir = snapshot_download(
            repo_id=repo_id,
            allow_patterns=[filename],
            local_files_only=True,
        )
    except LocalEntryNotFoundError:
        return None

    matches = glob.glob(os.path.join(snapshot_dir, filename))
    return matches[0] if matches else None


class Engine:
    def __init__(
        self,
        repo_id: str = "Qwen/Qwen2.5-3B-Instruct-GGUF",
        filename: str = "*q4_k_m.gguf",
        seed: int = -1,
    ):
        model_path = resolve_cached_model_path(repo_id, filename)

        if model_path is not None:
            self.llm = Llama(
                model_path=model_path,
                n_gpu_layers=-1,  # Auto-detects Metal, CUDA, or CPU
                seed=seed,
                verbose=False,
                n_ctx=2048,
            )
        else:
            # Not cached: download and populate the cache
            self.llm = Llama.from_pretrained(
                repo_id=repo_id,
                filename=filename,
                n_gpu_layers=-1,
                seed=seed,
                verbose=False,
                n_ctx=2048,
            )

        # Pre-decode the entire vocabulary for instant access
        print("Caching vocabulary...")
        vocab_size = self.llm.n_vocab()
        self.vocab_strings = []
        for i in range(vocab_size):
            # Decode each token safely, handling special/control tokens
            self.vocab_strings.append(
                self.llm.detokenize([i]).decode("utf-8", errors="ignore")
            )
        for i in range(self.llm.n_vocab()):
            decoded = self.llm.detokenize([i]).decode("utf-8", errors="ignore")

            if i < 20:
                print(i, repr(decoded))

    def tokenize(self, text: str) -> list[int]:
        return self.llm.tokenize(text.encode("utf-8"))

    def decode(self, tokens: list[int]) -> str:
        return self.llm.detokenize(tokens).decode("utf-8", errors="ignore")

    def prefill(
        self,
        prompt_text: str,
        logits_processor: LogitsProcessor | None = None,
        temperature: float = 0.0,
    ) -> DecodeState:
        """Tokenizes the prompt and returns a state tracker for stepping token-by-token."""
        prompt_tokens = self.tokenize(prompt_text)

        processors = LogitsProcessorList()

        if logits_processor is not None:
            processors.append(logits_processor)

        # Masking sets invalid tokens to -inf before temperature scaling is applied
        gen = self.llm.generate(
            prompt_tokens,
            logits_processor=processors,
            temp=temperature,
        )

        return DecodeState(step_generator=gen)

    def step(self, state: DecodeState) -> int | None:
        """Advances by one token; returns None on EOS or generation limit."""
        try:
            token = next(state.step_generator)

            if token == self.llm.token_eos():
                return None

            state.generated_tokens.append(token)
            return token

        except StopIteration:
            return None
