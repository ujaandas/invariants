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
            # Not cached yet: fall back to the network-aware loader, which
            # downloads and populates the cache for next time.
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
        """Convert a string into a list of token IDs"""
        return self.llm.tokenize(text.encode("utf-8"))

    def decode(self, tokens: list[int]) -> str:
        """Convert a list of token IDs back into a UTF-8 string"""
        return self.llm.detokenize(tokens).decode("utf-8", errors="ignore")

    def prefill(
        self,
        prompt_text: str,
        logits_processor: LogitsProcessor | None = None,
        temperature: float = 0.0,
    ) -> DecodeState:
        """
        Tokenizes the prompt, primes the KV cache, and returns a state tracker
        ready to step token-by-token.
        """
        prompt_tokens = self.tokenize(prompt_text)

        processors = LogitsProcessorList()

        if logits_processor is not None:
            processors.append(logits_processor)

        # Manages the KV cache state internally
        gen = self.llm.generate(
            prompt_tokens,
            logits_processor=processors,
            # Masking sets invalid tokens to -inf before any temperature
            # scaling, so the constraint guarantee holds at any temperature
            # -- greedy (0.0) is just the default, not a requirement.
            temp=temperature,
        )

        return DecodeState(step_generator=gen)

    def step(self, state: DecodeState) -> int | None:
        """
        Advances the model by exactly one token.
        Returns the new token ID, or None if EOS or generation limits are reached.
        """
        try:
            token = next(state.step_generator)

            # Check if we hit the EOS token
            if token == self.llm.token_eos():
                return None

            state.generated_tokens.append(token)
            return token

        except StopIteration:
            return None
