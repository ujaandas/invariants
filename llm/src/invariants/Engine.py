from llama_cpp import Llama, LogitsProcessorList, LogitsProcessor
from invariants.State import DecodeState


class Engine:
    def __init__(
        self,
        repo_id: str = "Qwen/Qwen2.5-3B-Instruct-GGUF",
        filename: str = "*q4_k_m.gguf",
        seed: int = -1,
    ):
        self.llm = Llama.from_pretrained(
            repo_id=repo_id,
            filename=filename,
            n_gpu_layers=-1,  # Auto-detects Metal, CUDA, or CPU
            seed=seed,
            local_files_only=True,
            verbose=False,
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
        temperature: float = 0.7,
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
            temp=temperature,  # Greedy decoding is mandatory for strict validation
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
