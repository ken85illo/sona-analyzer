const path = require('path')
const express = require('express')
const cors = require('cors')
const app = express()

function processLexemeAndTokens(output) {
    console.log(output)
    return JSON.parse(output)['lexical']
}

const { spawn } = require('child_process')

const exePath = path.join(__dirname, '../bin/Release/sona_lexical_analyzer')

// Middleware for parsing JSON
app.use(express.json({ limit: '10mb' }))

app.use(cors())
// Enable CORS for all routes and origins

// POST - lexical analyzer endpoint
app.post('/api/lexical-analyzer', (req, res) => {
    // Text area value - adjust if text area DTO changes
    const text = req.body.text

    // Insert cross communication with CPP executable
    const cpp = spawn(exePath)

    let output = ''

    cpp.stdin.write(text + '\n')
    // console.log(text)

    cpp.stdin.end()

    cpp.stdout.on('data', (data) => {
        output += data.toString()
    })

    cpp.stderr.on('data', (data) => {
        console.error(`C++ error: ${data}`)
    })

    cpp.on('close', (code) => {
        const processedOutput = processLexemeAndTokens(output)
        console.log(`C++ program exited with code ${code}`)
        res.json(processedOutput)
    })

    cpp.on('error', (err) => {
        console.error('Error running C++ program:', err)
        res.status(500).json({ error: 'Failed to run C++ program' })
    })
})

// Localhost port
const PORT = 8081

app.listen(PORT, () => {
    console.log('REST API server running on port 8081')
})
