const textarea_elem = document.getElementById("textarea-element");
const table_elem = document.getElementById("lexical-elements-table");
const lineSpinner = document.getElementById("line-number")
const defaultContent = table_elem.innerHTML
let lexicalAnalysis = null
let view = 0;
let highlightedLine = null;

const PORT = 8081;
const URL = `http://localhost:${PORT}/api/lexical-analyzer`;

const displayLexicalElements = () => {
    if (lexicalAnalysis === null) {
        return
    }
    lineSpinner.disabled = false;

    table_elem.innerHTML = defaultContent
    const filteredLines = lexicalAnalysis.filter((current) => current.line == lineSpinner.value)

    iterateLexicalAnalysis(filteredLines)
}

const displayAllLexicalElements = () => {
    if (lexicalAnalysis === null) {
        return
    }
    lineSpinner.disabled = true;
    // Reset table to default header
    table_elem.innerHTML = defaultContent;

    iterateLexicalAnalysis(lexicalAnalysis)
}

const iterateLexicalAnalysis = (analysis) => {
    let i = 0;

    for (const line of analysis) {
        const line_number = line.line;

        for (const lexical_element of line.elements) {
            const color = i % 2 ? 'td-color-1' : 'td-color-2';

            const table_row = `
                <tr>
                    <td class = "${color}">${line_number}</td>
                    <td class = "${color}">${lexical_element.token}</td>
                    <td class = "${color}">${lexical_element.lexeme}</td>
                </tr>
            `;
            i++;
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
        displayAllLexicalElements(); //display all elements
        //displayLexicalElements();
        console.error("Fetch error:", err);
    } catch (err) {
    }
};

//used codemirror as the editor alias
const editor = CodeMirror.fromTextArea(
    document.getElementById('textarea-element'),
    {
        lineNumbers: true,
        mode: 'text/x-csrc',
        theme: "seti",
        tabSize: 4,
        indentUnit: 4,
        indentWithTabs: false,
    }
)

const switchView = () => {
    if (view === 0) {
        displayLexicalElements();
        highlightEditorLine(lineSpinner.value);
        view = 1;
        return;
    }
    editor.removeLineClass(highlightedLine, "background", "codemirror_highlight");
    displayAllLexicalElements();
    view = 0;
}

const handleSubmit = () => {

    // Provide format of JSON to be sent
    const text_JSON = {
        text: editor.getValue()//replaced the element
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
    displayLexicalElements();
    highlightEditorLine(lineSpinner.value);
})

function highlightEditorLine(lineNumber) {
    const lineIndex = Number(lineNumber) - 1;

    if (highlightedLine !== null) {
        editor.removeLineClass(highlightedLine, "background", "codemirror_highlight");
    }

    editor.addLineClass(lineIndex, "background", "codemirror_highlight");

    highlightedLine = lineIndex;
    editor.scrollIntoView({ line: lineIndex, ch: 0 }, 100);
}

function insertSample() {
    editor.setValue(` //sample code        
int sum(int a, int b);
Machine SumMachine = {
    @context = {string input}
    @states = {"Idle", "Compute"};
    @start = "Idle";

    @transitions = {
        ("Idle", input == "compute") = "Compute";
        ("Compute", input == "done") = "Idle";
    }

    @state Idle = {
        print("Type 'compute' to calculate sum.");
    }

    @state Compute = {
        print("Enter first number:");
        int a = parseInt(readLine());
        print("Enter second number:");
        int b = parseInt(readLine());

        int result = sum(a, b);
        print(result);

        input = "done"
    }
}

SumMachine basicAdder;

int sum(int a, int b) {
    return a + b;
}

int main() {
    for(int i = 0 ; i < 5 ; i++) {
        string userInput = readLine();

        if(userInput == "exit") {
            break;
        } else {
            basicAdder.input = userInput;
        }
    }

    return 0;
}
Machine CoffeeMachine = {
    @context = {string button}
    @states = {"Idle", "Brewing", "Done"};
    @start = "Idle";

    @transitions = {
        ("Idle", button == "brew") = "Brewing";
        ("Brewing", button == "finish") = "Done";
        ("Done", button == "reset") = "Idle";
    }

    @state Idle = {
        print("Waiting for user...");
    }

    @state Brewing = {
        print("Brewing coffee...");
    }

    @state Done = {
        print("Coffee ready!");
    }
}

CoffeeMachine starbucks;

int main() {
    string input = "";
    bool active = true;

    while(active) {
        input = readLine();
        starbucks.button = input;
    }

    return 0;
}
`);
}

function spinnerIncrement() {
    lineSpinner.value = Number(lineSpinner.value) + 1;

    displayLexicalElements();
    highlightEditorLine(lineSpinner.value);
}

function spinnerDecrement() {
    if (lineSpinner.value <= 1)
        return;
    lineSpinner.value = Number(lineSpinner.value) - 1;

    displayLexicalElements();
    highlightEditorLine(lineSpinner.value);
}
