const textarea_elem = document.getElementById('textarea-element')
const table_elem = document.getElementById('lexical-elements-table')
const loadingIndicator = document.getElementById('loading-indicator')
const lineSpinner = document.getElementById('line-number')
const importBtn = document.getElementById('import-btn')
const syntaxContainer = document.getElementById('syntax-container')
const defaultContent = table_elem.innerHTML
let view = 0
let highlightedLine = null

const PORT = 8081
const URL = `http://localhost:${PORT}/api/lexical-analyzer`
lineSpinner.disabled = true

const displayLexicalElements = (lexicalAnalysis) => {
    if (lexicalAnalysis === null) return

    let html = defaultContent

    for (const [line, elements] of Object.entries(lexicalAnalysis)) {
        for (const lexical_element of elements) {
            html += `
                <tr data-line="${line}">
                    <td>${line}</td>
                    <td>${lexical_element.token}</td>
                    <td>${lexical_element.lexeme}</td>
                </tr>
            `
        }
    }

    table_elem.innerHTML = html
}

const displaySyntaxElements = (syntaxAnalysis, errors) => {
    if (syntaxAnalysis === null) return

    let html = ''

    for (const elem of syntaxAnalysis) {
        if (elem.type === 'terminal') {
            html += `
                <div class="syntax-card terminal-card" data-line="${elem.line}">
                    <div class="terminal-card-header">
                        <div>${elem.token_type}</div>
                        <div class="terminal-lexeme">${elem.lexeme}</div>
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
                    <div class="line-number">LINE ${elem.line}</div>
                </div>
            `
        } else if (elem.type === 'epsilon') {
            html += `
                <div class="syntax-card epsilon-card" data-line="${elem.line}">
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
                    <div class="line-number">LINE ${elem.line}</div>
                </div>
            `
        } else if (elem.type === 'error') {
            const sync = elem.synchronize
            html += `
                <div class="syntax-card error-card" data-line="${elem.line}">
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
                    <div class="line-number">LINE ${elem.line}</div>
                </div>
            `
        }
    }

    syntaxContainer.innerHTML = html
}

const lexicalAnalyzer = async (text_JSON) => {
    try {
        view = 0
        lineSpinner.disabled = true
        loadingIndicator.style.display = 'block'
        table_elem.innerHTML = ''
        syntaxContainer.innerHTML = ''

        const rawResponse = await fetch(URL, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(text_JSON),
        })

        if (!rawResponse.ok) throw new Error('Server error')

        const response = await rawResponse.json()
        const lexicalAnalysis = response['lexical']
        const syntaxAnalysis = response['syntactical']
        const errors = response['errors']
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
        displayLexicalElements(lexicalAnalysis) //display all elements
        displaySyntaxElements(syntaxAnalysis, errors)
        filterByLineAndType()
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
        highlightEditorLine(lineSpinner.value)
        lineSpinner.disabled = false
        view = 1

        filterByLineAndType()
        return
    }

    editor.removeLineClass(
        highlightedLine,
        'background',
        'codemirror_highlight'
    )
    lineSpinner.disabled = true
    view = 0
    filterByLineAndType()
}

const filterByLineAndType = (syntaxOnly = false) => {
    const line = lineSpinner.value
    const filter = document.querySelector(
        'input[name="syntax-filter"]:checked'
    ).value

    const cardElements = document.querySelectorAll('.syntax-card')
    let allElements = cardElements

    if (!syntaxOnly) {
        const tableElements = document.querySelectorAll('tr[data-line]')
        allElements = [...tableElements, ...cardElements]
    }

    const showAllLines = view === 0

    allElements.forEach((elem) => {
        const matchesLine = showAllLines || elem.dataset.line === line
        const matchesType =
            elem.tagName === 'TR' ||
            filter === 'all' ||
            (filter === 'productions' &&
                (elem.classList.contains('terminal-card') ||
                    elem.classList.contains('epsilon-card'))) ||
            (filter === 'errors' && elem.classList.contains('error-card'))

        if (matchesLine && matchesType) {
            elem.classList.remove('hidden')
        } else {
            elem.classList.add('hidden')
        }
    })
}

document.querySelectorAll('input[name="syntax-filter"]').forEach((radio) => {
    radio.addEventListener('change', () => {
        filterByLineAndType(true)
    })
})

lineSpinner.addEventListener('change', () => {
    filterByLineAndType()
    highlightEditorLine(lineSpinner.value)
})

const handleSubmit = () => {
    // Provide format of JSON to be sent
    const text_JSON = {
        text: editor.getValue(), //replaced the element
    }

    console.log('Handle Submit')

    lexicalAnalyzer(text_JSON)
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

    filterByLineAndType()
    highlightEditorLine(lineSpinner.value)
}

function spinnerDecrement() {
    if (lineSpinner.value <= 1) return
    lineSpinner.value = Number(lineSpinner.value) - 1

    filterByLineAndType()
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

document.getElementById('parse-console-btn').addEventListener('click', () => {
    filterByLineAndType()
    const parserLog = document.getElementById('parser-log')
    parserLog.show()

    const modalRect = parserLog.getBoundingClientRect()
    const x = (window.innerWidth - modalRect.width) / 2
    const y = (window.innerHeight - modalRect.height) / 2

    parserLog.style.transform = `translate(${x}px, ${y}px)`
    parserLog.dataset.x = x
    parserLog.dataset.y = y
})
