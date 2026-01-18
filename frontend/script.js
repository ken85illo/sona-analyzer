const textarea_elem = document.getElementById('textarea-element')
const table_elem = document.getElementById('lexical-elements-table')
const loadingIndicator = document.getElementById('loading-indicator')
const lineSpinner = document.getElementById('line-number')
const importBtn = document.getElementById('import-btn')
const parserDialog = document.getElementById('parser-log')
const syntaxContainer = document.getElementById('syntax-container')
const defaultContent = table_elem.innerHTML
let lexicalAnalysis = null
let syntaxAnalysis = null
let errors = null
let view = 0
let highlightedLine = null

const PORT = 8081
const URL = `http://localhost:${PORT}/api/lexical-analyzer`

const displayLexicalElements = (showAll = false) => {
    if (lexicalAnalysis === null) return

    lineSpinner.disabled = showAll

    let html = defaultContent

    if (showAll) {
        // Display all lines
        for (const [line, elements] of Object.entries(lexicalAnalysis)) {
            for (const lexical_element of elements) {
                html += `
                    <tr>
                        <td>${line}</td>
                        <td>${lexical_element.token}</td>
                        <td>${lexical_element.lexeme}</td>
                    </tr>
                `
            }
        }
    } else {
        // Display only the selected line
        const filteredLine = lexicalAnalysis[lineSpinner.value]
        if (!filteredLine) return

        for (const lexical_element of filteredLine) {
            html += `
                <tr>
                    <td>${lineSpinner.value}</td>
                    <td>${lexical_element.token}</td>
                    <td>${lexical_element.lexeme}</td>
                </tr>
            `
        }
    }

    table_elem.innerHTML = html
}

const displaySyntaxElements = () => {
    let html = ''

    for (const elem of syntaxAnalysis) {
        if (elem.type === 'terminal') {
            html += `
                <div class="syntax-card terminal-card">
                    <div class="terminal-card-header">
                        <div>${elem.token_type}</div>
                        <div>${elem.lexeme}</div>
                    </div>
                    <hr/>
                    <div class="syntax-card-prod">
            `

            for (const nonTerm of elem.non_terminals.split('|')) {
                html += `
                    <div>&lt;${nonTerm}&gt;</div>
                `
            }

            html += `
                    </div>
                    <div class="line-number">${elem.line}</div>
                </div>
            `
        } else if (elem.type === 'epsilon') {
            html += `
                <div class="syntax-card epsilon-card">
                    <div>EPSILON</div>
                    <hr/>
                    <div class="syntax-card-prod">
            `

            for (const nonTerm of elem.non_terminals.split('|')) {
                html += `
                    <div>&lt;${nonTerm}&gt;</div>
                `
            }

            html += `
                    </div>
                    <div class="line-number">${elem.line}</div>
                </div>
            `
        } else if (elem.type === 'error') {
            const sync = elem.synchronize
            html += `
                <div class="syntax-card error-card">
                    <div>ERROR</div>
                    <hr/>
                    <div class="error-message">${errors[String(elem.line)][elem.index]}</div>
            `

            if (sync) {
                html += `
                    <div class="error-message margin-bottom">Synchronizing to token ${sync.token_type} with lexeme '${sync.lexeme}' on line ${sync.line}... </div>
                `
            }

            html += `
                    <div class="syntax-card-prod">
            `

            for (const nonTerm of elem.non_terminals.split('|')) {
                html += `
                    <div>&lt;${nonTerm}&gt;</div>
                `
            }

            html += `
                    </div>
                    <div class="line-number">${elem.line}</div>
                </div>
            `
        }
    }

    syntaxContainer.innerHTML = html
}

const lexicalAnalyzer = async (text_JSON) => {
    try {
        loadingIndicator.style.display = 'block'
        table_elem.innerHTML = ''

        const rawResponse = await fetch(URL, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(text_JSON),
        })

        if (!rawResponse.ok) throw new Error('Server error')

        const response = await rawResponse.json()
        lexicalAnalysis = response['lexical']
        syntaxAnalysis = response['syntactical']
        errors = response['errors']
        console.log(lexicalAnalysis)

        lineSpinner.value = 1
        lineSpinner.max = String(
            Object.keys(lexicalAnalysis).reduce((max, current) => {
                const currentLine = parseInt(current)
                if (currentLine > max) {
                    return currentLine
                }
                return max
            }, 0)
        )
        displayLexicalElements(true) //display all elements
        displaySyntaxElements()
    } catch (err) {
        console.error('Fetch error:', err)
    } finally {
        loadingIndicator.style.display = 'none'
    }
}

//used codemirror as the editor alias
const editor = CodeMirror.fromTextArea(
    document.getElementById('textarea-element'),
    {
        lineNumbers: true,
        mode: 'text/x-csrc',
        theme: 'seti',
        tabSize: 4,
        indentUnit: 4,
        indentWithTabs: false,
    }
)

const switchView = () => {
    if (view === 0) {
        displayLexicalElements()
        highlightEditorLine(lineSpinner.value)
        view = 1
        return
    }
    editor.removeLineClass(
        highlightedLine,
        'background',
        'codemirror_highlight'
    )
    displayLexicalElements(true)
    view = 0
}

const handleSubmit = () => {
    // Provide format of JSON to be sent
    const text_JSON = {
        text: editor.getValue(), //replaced the element
    }

    console.log('Handle Submit')

    lexicalAnalyzer(text_JSON)
    parserDialog.showModal()
}

// Adds tabs instead of manually adding white-spaces
textarea_elem.addEventListener('keydown', (e) => {
    if (e.key === 'Tab') {
        e.preventDefault()

        const start = textarea_elem.selectionStart
        const end = textarea_elem.selectionEnd

        // Insert tab at cursor position
        textarea_elem.value =
            textarea_elem.value.substring(0, start) +
            '\t' +
            textarea_elem.value.substring(end)

        // Move the cursor after the inserted tab
        textarea_elem.selectionStart = textarea_elem.selectionEnd = start + 1
    }
})

lineSpinner.addEventListener('change', (e) => {
    displayLexicalElements()
    highlightEditorLine(lineSpinner.value)
})

function highlightEditorLine(lineNumber) {
    const lineIndex = Number(lineNumber) - 1

    if (highlightedLine !== null) {
        editor.removeLineClass(
            highlightedLine,
            'background',
            'codemirror_highlight'
        )
    }

    editor.addLineClass(lineIndex, 'background', 'codemirror_highlight')

    highlightedLine = lineIndex
    editor.scrollIntoView({ line: lineIndex, ch: 0 }, 100)
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
    @context = {int temperature}

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
    `)
}

function spinnerIncrement() {
    if (Number(lineSpinner.value) >= lineSpinner.max) return

    lineSpinner.value = Number(lineSpinner.value) + 1

    displayLexicalElements()
    highlightEditorLine(lineSpinner.value)
}

function spinnerDecrement() {
    if (lineSpinner.value <= 1) return
    lineSpinner.value = Number(lineSpinner.value) - 1

    displayLexicalElements()
    highlightEditorLine(lineSpinner.value)
}

importBtn.addEventListener('change', () => {
    const reader = new FileReader()

    reader.onload = function () {
        editor.setValue(reader.result)
    }

    reader.onerror = function (error) {
        alert(`Error reading file: ${error.type}`)
    }

    if (importBtn.files && importBtn.files[0]) {
        console.log(importBtn.files)
        reader.readAsText(importBtn.files[0])
    }
})
