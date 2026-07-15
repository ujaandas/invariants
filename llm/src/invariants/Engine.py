from llama_cpp import Llama, LogitsProcessorList, LogitsProcessor
from invariants.State import DecodeState


class Engine:
    def __init__(
        self,
        repo_id: str = "Qwen/Qwen2.5-3B-Instruct-GGUF",
        filename: str = "*q4_k_m.gguf",
    ):
        self.llm = Llama.from_pretrained(
            repo_id=repo_id,
            filename=filename,
            n_gpu_layers=-1,  # Auto-detects Metal, CUDA, or CPU
            verbose=False,
        )

    def prefill(self, prompt_text: str, processor: LogitsProcessor) -> DecodeState:
        """
        Initializes the context and returns the generator state.
        """
        prompt_tokens = self.llm.tokenize(
            prompt_text.encode("utf-8"), add_special_tokens=False
        )
        processor_list = LogitsProcessorList([processor])

        # Create the generator. KV cache is updated on every next() call.
        gen = self.llm.generate(
            prompt_tokens,
            logits_processor=processor_list,
            temp=0.0,  # Greedy decoding for structured generation
        )

        return DecodeState(step_generator=gen, generated_tokens=[])

    def step(self, state: DecodeState) -> int | None:
        """
        Advances the model by exactly one token.
        """
        try:
            token = next(state.step_generator)

            if token == self.llm.token_eos():
                return None

            state.generated_tokens.append(token)
            return token

        except StopIteration:
            return None
