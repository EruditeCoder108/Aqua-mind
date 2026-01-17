/**
 * Aqua-Mind Gemini AI Integration
 * ================================
 * Provides AI-powered water quality analysis using Google's Gemini API.
 */

class GeminiAnalyzer {
    static API_ENDPOINT = 'https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent';

    constructor(apiKey = null) {
        this.apiKey = apiKey;
        this.lastAnalysis = null;
    }

    /**
     * Set API key
     */
    setApiKey(key) {
        this.apiKey = key;
    }

    /**
     * Check if API key is configured
     */
    hasApiKey() {
        return !!this.apiKey;
    }

    /**
     * Analyze water quality data using Gemini
     */
    async analyze(data) {
        if (!this.apiKey) {
            throw new Error('Gemini API key not configured');
        }

        const prompt = this._buildPrompt(data);

        try {
            const response = await fetch(`${GeminiAnalyzer.API_ENDPOINT}?key=${this.apiKey}`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    contents: [{
                        parts: [{ text: prompt }]
                    }],
                    generationConfig: {
                        temperature: 0.3,
                        topP: 0.8,
                        maxOutputTokens: 500
                    }
                })
            });

            if (!response.ok) {
                const error = await response.json();
                throw new Error(error.error?.message || 'API request failed');
            }

            const result = await response.json();
            const text = result.candidates?.[0]?.content?.parts?.[0]?.text;

            if (!text) {
                throw new Error('No response from Gemini');
            }

            // Parse the JSON response
            const analysis = this._parseResponse(text);
            this.lastAnalysis = analysis;

            return analysis;
        } catch (error) {
            console.error('Gemini analysis failed:', error);

            // Return fallback analysis
            return this._getFallbackAnalysis(data);
        }
    }

    /**
     * Build the prompt for Gemini
     */
    _buildPrompt(data) {
        return `You are Aqua-Mind, a water quality expert for the Indian Jal Jeevan Mission.
You analyze water quality data and provide simple, actionable advice.

Context:
- Location: ${data.profile || 'India'}
- TDS: ${data.tds} ppm (Total Dissolved Solids)
- Turbidity: ${data.turb} NTU (Nephelometric Turbidity Units)
- Temperature: ${data.temp}°C
- Sensor Stability: ${data.stability}%
- Jal-Score: ${data.score}/100
- Initial Verdict: ${data.verdict}

Strict Rules:
1. If Stability < 60%: Focus on sensor error, suggest cleaning probe
2. If Turbidity > 5 NTU: Flag as 'Microbial Risk Condition'
3. DO NOT give medical prescriptions
4. Use simple Hindi/English mix where appropriate
5. Be concise and practical

Respond ONLY with valid JSON in this exact format (no markdown, no code blocks):
{
  "status": "Safe" or "Caution" or "Unsafe",
  "headline": "Short 5-7 word summary in Hindi/English",
  "explanation": "Simple 2-3 sentence explanation of what the readings mean, using everyday analogies",
  "action": "One specific, practical filtration or treatment step the user should take"
}`;
    }

    /**
     * Parse Gemini response
     */
    _parseResponse(text) {
        // Try to extract JSON from the response
        let jsonStr = text;

        // Remove markdown code blocks if present
        const jsonMatch = text.match(/```(?:json)?\s*([\s\S]*?)```/);
        if (jsonMatch) {
            jsonStr = jsonMatch[1];
        }

        // Try to find JSON object
        const objectMatch = jsonStr.match(/\{[\s\S]*\}/);
        if (objectMatch) {
            jsonStr = objectMatch[0];
        }

        try {
            const parsed = JSON.parse(jsonStr.trim());

            // Validate required fields
            return {
                status: parsed.status || 'Unknown',
                headline: parsed.headline || 'Analysis complete',
                explanation: parsed.explanation || 'Please review the readings.',
                action: parsed.action || 'Consult local water authority if concerned.'
            };
        } catch (e) {
            console.warn('Failed to parse Gemini response:', e);
            console.log('Raw response:', text);

            // Try to extract useful info from unstructured response
            return {
                status: 'Unknown',
                headline: 'Analysis complete',
                explanation: text.substring(0, 200),
                action: 'Please review the sensor readings.'
            };
        }
    }

    /**
     * Get fallback analysis when API fails
     */
    _getFallbackAnalysis(data) {
        const verdict = data.verdict?.toUpperCase() || 'UNKNOWN';

        const fallbacks = {
            SAFE: {
                status: 'Safe',
                headline: 'पानी पीने योग्य है (Water is safe)',
                explanation: 'All readings are within safe limits. TDS and turbidity levels indicate good water quality.',
                action: 'Store in clean containers and consume within 24 hours.'
            },
            CAUTION: {
                status: 'Caution',
                headline: 'पानी में थोड़ी समस्या (Minor water issues)',
                explanation: `TDS of ${data.tds}ppm is slightly elevated. This means the water has more dissolved minerals than ideal.`,
                action: 'Use a basic water filter or boil before drinking for added safety.'
            },
            UNSAFE: {
                status: 'Unsafe',
                headline: 'पानी असुरक्षित है (Water is unsafe)',
                explanation: `Turbidity of ${data.turb} NTU indicates possible contamination. High turbidity can hide harmful microbes.`,
                action: 'DO NOT drink directly. Boil for 10 minutes or use RO/UV purifier.'
            },
            ERROR: {
                status: 'Error',
                headline: 'सेंसर त्रुटि (Sensor error)',
                explanation: `Stability at ${data.stability}% is too low for reliable readings. The sensor may be dirty or malfunctioning.`,
                action: 'Clean the sensor probe with soft cloth and test again.'
            }
        };

        return fallbacks[verdict] || fallbacks.CAUTION;
    }

    /**
     * Get cached analysis
     */
    getLastAnalysis() {
        return this.lastAnalysis;
    }

    /**
     * Check if Gemini API is reachable
     */
    async testConnection() {
        if (!this.apiKey) {
            return { success: false, error: 'API key not set' };
        }

        try {
            const response = await fetch(`${GeminiAnalyzer.API_ENDPOINT}?key=${this.apiKey}`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    contents: [{ parts: [{ text: 'Say "OK" if you can read this.' }] }],
                    generationConfig: { maxOutputTokens: 10 }
                })
            });

            if (response.ok) {
                return { success: true };
            } else {
                const error = await response.json();
                return { success: false, error: error.error?.message || 'Unknown error' };
            }
        } catch (e) {
            return { success: false, error: e.message };
        }
    }
}


// Export for use in app.js
window.GeminiAnalyzer = GeminiAnalyzer;
