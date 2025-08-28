document.getElementById('camera-stream').onload = function() {
    document.querySelector('.container').style.display = 'none';
    this.style.display = 'block';
};

const temperatureElement = document.getElementById('temperature-value');
const humidityElement = document.getElementById('humidity-value');

function fetchDhtData() {
    fetch('/dht')
        .then(response => response.json())
        .then(data => {
            if (data.error) {
                temperatureElement.textContent = 'Erreur';
                humidityElement.textContent = 'Erreur';
            } else {
                temperatureElement.textContent = data.temperature.toFixed(1);
                humidityElement.textContent = data.humidity.toFixed(1);
            }
        })
        .catch(error => {
            console.error('Erreur de requête:', error);
            temperatureElement.textContent = 'Erreur';
            humidityElement.textContent = 'Erreur';
        });
}

setInterval(fetchDhtData, 5000);
fetchDhtData();

// --- Logique pour les boutons RGB ---
let colorState = { r: 0, g: 0, b: 0 };
let lastColor = { r: 255, g: 255, b: 255 };

function updateLedColor() {
    fetch(`/led?r=${colorState.r}&g=${colorState.g}&b=${colorState.b}`);
}

function updateButtonState() {
    document.getElementById('btn-red').classList.toggle('active', colorState.r > 0);
    document.getElementById('btn-green').classList.toggle('active', colorState.g > 0);
    document.getElementById('btn-blue').classList.toggle('active', colorState.b > 0);
    
    // Le bouton OFF est actif si toutes les couleurs sont éteintes
    document.getElementById('btn-off').classList.toggle('active', colorState.r === 0 && colorState.g === 0 && colorState.b === 0);
}

// APPEL DE LA FONCTION AU DÉMARRAGE
// Cela garantit que le bouton est désactivé visuellement dès le chargement de la page.
updateButtonState();

// Clics sur les boutons de couleur
document.getElementById('btn-red').addEventListener('click', () => {
    colorState.r = colorState.r > 0 ? 0 : 255;
    lastColor = { ...colorState };
    updateLedColor();
    updateButtonState();
});

document.getElementById('btn-green').addEventListener('click', () => {
    colorState.g = colorState.g > 0 ? 0 : 255;
    lastColor = { ...colorState };
    updateLedColor();
    updateButtonState();
});

document.getElementById('btn-blue').addEventListener('click', () => {
    colorState.b = colorState.b > 0 ? 0 : 255;
    lastColor = { ...colorState };
    updateLedColor();
    updateButtonState();
});

// Clic sur le bouton d'alimentation
document.getElementById('btn-off').addEventListener('click', () => {
    const isOff = colorState.r === 0 && colorState.g === 0 && colorState.b === 0;
    
    if (isOff) {
        // Rallumer à la dernière couleur si c'est éteint
        colorState = { ...lastColor };
    } else {
        // Éteindre
        lastColor = { ...colorState };
        colorState = { r: 0, g: 0, b: 0 };
    }
    updateLedColor();
    updateButtonState();
});