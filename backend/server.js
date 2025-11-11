const path = require('path');
const express = require('express');
const cors = require('cors');
const app = express();

function processLexemeAndTokens(output) {
    // g indicates a global search, d indicates the index of that string match
    const find_line_number_regex = /\[\d+\],/gd;

    /* 
        Matches the part of the string that indicates the current line of code (e.g. [2], [10]).
        It then stores it in this CODE_LINE_INDICES_LIST list

    */
    const CODE_LINE_INDICES_LIST = [];

    for (const match of output.matchAll(find_line_number_regex)) {    
        CODE_LINE_INDICES_LIST.push({
            current_line: match[0],
            index: match.indices[0][0],
            line_last_index: 0    
        })
    }

    // Sets upper boundary of the line. Indicates up to what index a line consumes.
    for(let i = CODE_LINE_INDICES_LIST.length - 1 ; i > 0 ; i--) {
        let current_line_object = CODE_LINE_INDICES_LIST.at(i);

        if(i === CODE_LINE_INDICES_LIST.length - 1) {
            current_line_object.line_last_index = output.length-1;
        }

        // Gets the line object stored at index i - 1 in CODE_LINE_INDICES_LIST 
        let next_line_object = CODE_LINE_INDICES_LIST.at(i - 1);

        next_line_object.line_last_index = current_line_object.index - 1;
    }

    // Regex for locating the beginning of a token/lexeme
    const tokens_and_lexeme_regex = /\#\d+/gd;

    // List to hold token/lexeme
    const TOKENS_AND_LEXEME_LIST = [];

    for (const match of output.matchAll(tokens_and_lexeme_regex)) {    
        // Parses the pattern to an int. #7 -> 7, #15 -> 15.
        const token_lexeme_length = 0 + parseInt(output.substring(match.indices[0][0] + 1, match.indices[0][1]));
        
        // Gets the actual token/lexeme string
        const token_lexeme = output.substring(match.indices[0][1] + 1, match.indices[0][1] + token_lexeme_length + 1);

        TOKENS_AND_LEXEME_LIST.push({
            token_lexeme: token_lexeme,
            index: match.indices[0][0] // Will be used to check what line the token/lexeme belongs to
        })
    }

    //  Dissects the tokens and lexemes into their respective lines
    const LINE_TOKENS_AND_LEXEMES_JSON = [];

    // Tracks the current token/lexeme from the TOKEN
    let token_lexeme_counter = 0;

    for(const line of CODE_LINE_INDICES_LIST) {
        
        // Placeholder object to be stored in 
        const obj = {
            line: line.current_line.substring(1, line.current_line.length - 2), // [2] -> 2
            elements: [] 
        };

        // Gets the index of where the line ends. Example: Line 1 of the program ends at index 17.
        let end_of_line = line.line_last_index;

        // Grabs the first token
        let current_token_lexeme = TOKENS_AND_LEXEME_LIST.at(token_lexeme_counter);
        
        // Iterates until it visits all token/lexemes stored at TOKENS_AND_LEXEME_LIST, or it reaches the end of the line index
        while(token_lexeme_counter < TOKENS_AND_LEXEME_LIST.length && current_token_lexeme.index < end_of_line) {
            const lexical_elements = {
                token: current_token_lexeme.token_lexeme, // Sets the token
                lexeme: ""
            };
            
            token_lexeme_counter++;
            current_token_lexeme = TOKENS_AND_LEXEME_LIST.at(token_lexeme_counter); // Grabs the lexeme
            
            lexical_elements.lexeme = current_token_lexeme.token_lexeme; // Sets the lexeme

            obj.elements.push(lexical_elements);

            // Iterates to the next token and lexeme pair1
            token_lexeme_counter++;
            current_token_lexeme = TOKENS_AND_LEXEME_LIST.at(token_lexeme_counter); // Grabs the token for the next iteration
        }

        LINE_TOKENS_AND_LEXEMES_JSON.push(obj);

    }

    for(const line of LINE_TOKENS_AND_LEXEMES_JSON) {
        console.log(line);
    }

    return LINE_TOKENS_AND_LEXEMES_JSON;
}

const { spawn } = require('child_process');

const exePath = path.join(__dirname, '../bin/Release/sona_lexical_analyzer');

// Middleware for parsing JSON
app.use(express.json());

app.use(cors());
// Enable CORS for all routes and origins

// POST - lexical analyzer endpoint
app.post('/api/lexical-analyzer', (req, res) => {
    // Text area value - adjust if text area DTO changes
    const text = req.body.text;

    // Insert cross communication with CPP executable
    const cpp = spawn(exePath)

    let output = ""

    cpp.stdin.write(text + '\n');
    // console.log(text)
    
    cpp.stdin.end();

    cpp.stdout.on('data', (data) => {
        output += data.toString();
    });

    cpp.stderr.on('data', (data) => {
        console.error(`C++ error: ${data}`);
    });

    cpp.on('close', (code) => {
        const processedOutput = processLexemeAndTokens(output);
        console.log(`C++ program exited with code ${code}`);
        res.json(processedOutput);
    });

    cpp.on('error', (err) => {
        console.error('Error running C++ program:', err);
        res.status(500).json({ error: 'Failed to run C++ program' });
    });
})

// Localhost port
const PORT = 8081;

app.listen(PORT, () => {
    console.log('REST API server running on port 8081');
});
