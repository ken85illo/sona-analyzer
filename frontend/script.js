const textarea_elem = document.getElementById("textarea-element");
const table_elem = document.getElementById("lexical-elements-table");
const lineSpinner = document.getElementById("line-number")
const defaultContent = table_elem.innerHTML
let lexicalAnalysis = null

const PORT = 8081;
const URL = `http://localhost:${PORT}/api/lexical-analyzer`;

const displayLexicalElements = () => {
    if (lexicalAnalysis === null) {
        return
    }

    table_elem.innerHTML = defaultContent
    const filteredLines = lexicalAnalysis.filter((current) => current.line == lineSpinner.value)

    for (const line of filteredLines) {
        const line_number = line.line;

        for (const lexical_element of line.elements) {
            const token_lexeme = `${lexical_element.token}: ${lexical_element.lexeme}`;

            const table_row = `
                <tr>
                    <td>${line_number}</td>
                    <td>${token_lexeme}</td>
                </tr>
            `;

            table_elem.innerHTML += table_row;
        }
    }
}

const lexicalAnalyzer = async (text_JSON) => {
    try {
        const rawResponse = await fetch(URL, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(text_JSON)
        });

        if (!rawResponse.ok) throw new Error("Server error");

        lexicalAnalysis = await rawResponse.json();
        lineSpinner.value = 1
        lineSpinner.max = String(lexicalAnalysis.reduce((max, current) => {
            const currentLine = parseInt(current.line)
            if (currentLine > max) {
                return currentLine
            }
            return max
        }, 0))
        console.log(lexicalAnalysis);
        displayLexicalElements();
        console.error("Fetch error:", err);
    } catch (err) {
    }
};

const handleSubmit = () => {

    // Provide format of JSON to be sent
    const text_JSON = {
        text: textarea_elem.value
    }

    console.log("Handle Submit");

    lexicalAnalyzer(text_JSON);
}

// Adds tabs instead of manually adding white-spaces
textarea_elem.addEventListener('keydown', (e) => {
    if (e.key === 'Tab') {
        e.preventDefault();

        const start = textarea_elem.selectionStart;
        const end = textarea_elem.selectionEnd;

        // Insert tab at cursor position
        textarea_elem.value =
            textarea_elem.value.substring(0, start) + "\t" + textarea_elem.value.substring(end);

        // Move the cursor after the inserted tab
        textarea_elem.selectionStart = textarea_elem.selectionEnd = start + 1;
    }
})

lineSpinner.addEventListener('change', (e) => {
    displayLexicalElements()
})
