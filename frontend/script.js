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
    const filteredLines = lexicalAnalysis[lineSpinner.value]

    if (filteredLines === undefined) {
        return
    }

    let html = "";
    let i = 0;

    // Reset table to default header
    html += defaultContent;

    for (const lexical_element of filteredLines) {
        const color = i % 2 ? 'td-color-1' : 'td-color-2';

        html += `
            <tr>
                <td class = "${color}">${lineSpinner.value}</td>
                <td class = "${color}">${lexical_element.token}</td>
                <td class = "${color}">${lexical_element.lexeme}</td>
            </tr>
        `;
        i++;

    }
    table_elem.innerHTML = html;
}

const displayAllLexicalElements = () => {
    if (lexicalAnalysis === null) {
        return
    }
    lineSpinner.disabled = true;

    let html = "";
    let i = 0;

    // Reset table to default header
    html += defaultContent;

    for (const [line, elements] of Object.entries(lexicalAnalysis)) {
        const line_number = line;

        for (const lexical_element of elements) {
            const color = i % 2 ? 'td-color-1' : 'td-color-2';

            html += `
                <tr>
                    <td class = "${color}">${line_number}</td>
                    <td class = "${color}">${lexical_element.token}</td>
                    <td class = "${color}">${lexical_element.lexeme}</td>
                </tr>
            `;
            i++;

        }
    }
    table_elem.innerHTML = html;
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
        lineSpinner.max = String(Object.entries(lexicalAnalysis).reduce((max, current) => {
            const currentLine = parseInt(current[0])
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
    editor.setValue(`void maintainTemperature(int currentTemperature) {
    // Add example maintaning mechanism
}

void increaseTemperature(int currentTemperature) {
    // Add example heating mechanism
}

void decreaseTemperature(int currentTemperature) {
    // Add example cooling mechanism
}

Machine Thermometer = {
    @context = {int temperature};

    @states = {"Freezing", "Cold", "Normal", "Hot", "Boiling"};
    @start = "Normal";
    @final = {"Freezing", "Boiling"};

    @transitions = {
        ("Normal", temperature <= 15) = "Cold";
        ("Normal", temperature >= 36) = "Hot";

        ("Cold", temperature > 15 && temperature < 36) = "Normal";
        ("Cold", temperature <= 0) = "Freezing";

        ("Freezing", temperature > 0) = "Cold";

        ("Hot", temperature > 15 && temperature < 36) = "Normal";
        ("Hot", temperature >= 100) = "Boiling";

        ("Boiling", temperature < 100) = "Hot";
    }

    @state Normal = {
        maintainTemperature(temperature);
        print("Temperature: " + temperature + "°C — Normal range.");
    }

    @state Cold = {
        decreaseTemperature(temperature);
        print("Temperature: " + temperature + "°C — It's cold.");
    }

    @state Freezing = {
        decreaseTemperature(temperature);
        print("Warning: Freezing temperature!");
        print("Temperature: " + temperature + "°C");
    }

    @state Hot = {
        increaseTemperature(temperature);
        print("Temperature: " + temperature + "°C — It's hot!");
    }

    @state Boiling = {
        increaseTemperature(temperature);
        print("Danger: Boiling temperature!");
        print("Temperature: " + temperature + "°C");
    }

    @finalState = {
        print("Final temperature state reached.");
    }
}

Thermometer thermometer;

int main() {
    bool isPowerOn = true;
    int temperatureInput = 0;

    while(isPowerOn) {
        print("Enter desired temperature: ");
        temperatureInput = parseInt(readLine());

        thermometer.temperature = temperatureInput;
    }
}
    `);
}

function spinnerIncrement() {
    if (Number(lineSpinner.value) >= lineSpinner.max)
        return;

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
