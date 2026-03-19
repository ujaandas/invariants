### Logit Masking

$M_{syn}$ needs to be aware of the model's vocabulary at every token.

Recall that a vocabulary is basically just the compelte "dictionary" of every possible
fragment the model can receive or produce. Because LLMs don't see words, but
rather tokens, a token can be a whole word, a fragment, or even a single
punctuation. For a model like Llama 3, the vocabulary size ($|V|$) is 128,256.

Also recall what a logit is. A logit is the raw, unnormalized "score" that the
model assigns each token in its vocabulary at that given moment in time. Before
the model decides what to say, its internal network produces a massive vector of
numbers, one for each token in the vocab. Logits can be any real number, so the
model uses a softmax function to turn them into percentages:

$$
softmax(z)_i = \frac{e^{z_i}}{\sum^{|V|}_{j=1} e^{z_j}}
$$

Thus, when we say we perform "logit masking", we are going inside the model, and
setting the logit of an illegal token to $-\infty$.

### Trie-Based Masking

The model has access to a vocabulary of length $|V|$, and we definitely cannot
afford to iterate through each of these at every token. Thus, we should
pre-process the tokenizer's vocabulary into a prefix trie. As the model "types"
characters, we move down the branches of the trie. Thus, we transform the
$O(|V|)$ to $O(L)$, where $L$ is the length of that path.

### Bitmask

Instead of sending a list of allowed words, we can just use a bitmask. We send a
giant array of $O(|V|)$, where each bit represents one of the many tokens. `1`
allows the token, and `0` doesn't.

### Dependency Graph

Invariants can be complex, so a graph allows us to more aptly represent some of
those complexities. For instance, given this invariant:

`total_cost = (price * quantity) * tax`

Here, we set the following:

- Vertices: `total_cost, price, quantity, tax`
- Edges: `price -> total_cost`, `quantity -> total_cost`, etc...

This lets us compute which properties rely on which. We cannot compute
`total_cost` until the other three fields are finished, so if the model were to
generate `total_cost` first, the $M_{syn}$ would obviously allow it, but thanks
to the dependency graph, we know $E_{sem}$ should keep an eye on it till the
other properties are fulfilled, then check it _after_ they are all ready.

Alternatively, $E_{sem}$ could "inform" or "suggest" an order to the model,
which would save computation in the event that a dependent value is generated
incorrectly.
