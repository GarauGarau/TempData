document.getElementById('update-data').addEventListener('click', function() {
    loadDataAndUpdateCharts();
});

function loadDataAndUpdateCharts() {
    const url = 'https://script.google.com/macros/s/AKfycbxd1U-hm2xo79srYB-o9AdgHBBCKOrbaL4fFzdJlXbzhpV08Sq8Tua6qk_5Q78cJWFZ/exec';  // Sostituisci con l'URL della tua web app

    fetch(url)
        .then(response => response.json())
        .then(data => {
            processAndDisplayData(data);
        })
        .catch(error => console.error('Errore durante il fetch dei dati:', error));
}

function parseDateString(dateString) {
    const [datePart, timePart] = dateString.split(', ');
    const [day, month, year] = datePart.split('/');
    const [hours, minutes, seconds] = timePart.split(':');

    return new Date(year, month - 1, day, hours, minutes, seconds);
}


function processAndDisplayData(data) {
    // Pre-process data
	const processedData = data.map(row => {
		let temperatura = row.Temperatura;
		if (typeof temperatura === 'string') {
			temperatura = parseFloat(temperatura.replace(',', '.'));
		} else if (typeof temperatura !== 'number') {
			console.error('Campo Temperatura non trovato o non valido:', row);
			temperatura = NaN;  // Segna come non valido se non è un numero o stringa
		}

		// Parsing della data
		const parsedDate = new Date(row.Data);
		if (isNaN(parsedDate.getTime())) {
			console.error('Data non valida:', row.Data);
		}

		return {
			...row,
			Data: parsedDate,  // Assicurati che questa data sia valida
			Temperatura: temperatura   // Usa il valore corretto della temperatura
		};
	});


    // Verifica se ci sono dati validi
    if (processedData.every(row => isNaN(row.Temperatura))) {
        console.error('Tutti i valori di temperatura sono non validi.');
        return;
    }

    // Filtraggio dei dati basato sul filtro selezionato per il primo grafico
    const timeFilter = document.getElementById('timeFilter').value;
    const filteredDataMinuto = filterDataByTime(processedData, timeFilter);
    
    if (filteredDataMinuto.length === 0) {
        console.error("Nessun dato disponibile dopo il filtraggio.");
        document.getElementById('latest-temp').textContent = "Nessun dato disponibile.";
        return;
    }

    // Latest temperature
    const latestTemp = filteredDataMinuto[filteredDataMinuto.length - 1].Temperatura;
    document.getElementById('latest-temp').textContent = `Ultima temperatura rilevata: ${latestTemp.toFixed(2)}°C`;

    // Last update time
    const lastUpdateTime = new Date().toLocaleString('it-IT', { hour12: false });
    document.getElementById('last-update').textContent = `Ultimo aggiornamento: ${lastUpdateTime}`;

    // Preparazione dei dati per il primo grafico (minuto per minuto)
    const labelsMinuto = filteredDataMinuto.map(row => row.Data.toLocaleString('it-IT'));
    const dataMinuto = filteredDataMinuto.map(row => row.Temperatura);

    // Preparazione dei dati per il secondo grafico (media oraria)
    const hourlyData = aggregateHourly(processedData);
    const labelsOra = hourlyData.map(row => row.label);
    const dataOra = hourlyData.map(row => row.meanTemp);
    const ciUpper = hourlyData.map(row => row.ciUpper);
    const ciLower = hourlyData.map(row => row.ciLower);

    // Aggiorna i grafici
    updateCharts(labelsMinuto, dataMinuto, labelsOra, dataOra, ciUpper, ciLower);
}



function filterDataByTime(data, filter) {
    const lastTimestamp = data[data.length - 1].Data.getTime();

    let timeFrame = 0;
    switch (filter) {
        case 'last24h':
            timeFrame = 24 * 60 * 60 * 1000; // 24 ore in millisecondi
            break;
        case 'last12h':
            timeFrame = 12 * 60 * 60 * 1000; // 12 ore in millisecondi
            break;
        case 'last6h':
            timeFrame = 6 * 60 * 60 * 1000;  // 6 ore in millisecondi
            break;
        case 'last3h':
            timeFrame = 3 * 60 * 60 * 1000;  // 3 ore in millisecondi
            break;
        default:
            return data; // Se è selezionato "tutti", restituisci tutti i dati
    }

    const filteredData = data.filter(row => row.Data.getTime() >= (lastTimestamp - timeFrame));
    console.log(`Dati filtrati (${filter}):`, filteredData);
    return filteredData;
}



function aggregateHourly(data) {
    const grouped = data.reduce((acc, curr) => {
        const hour = curr.Data.getHours();
        const date = curr.Data.toLocaleDateString('it-IT');
        const key = `${date} ${hour}:00`;

        if (!acc[key]) {
            acc[key] = [];
        }

        acc[key].push(curr.Temperatura);
        return acc;
    }, {});

    return Object.keys(grouped).map(key => {
        const values = grouped[key];
        const meanTemp = values.reduce((a, b) => a + b, 0) / values.length;
        const stdErr = Math.sqrt(values.reduce((sum, val) => sum + Math.pow(val - meanTemp, 2), 0) / (values.length - 1)) / Math.sqrt(values.length);
        const ci95 = 1.96 * stdErr;

        return {
            label: key,
            meanTemp,
            ciUpper: meanTemp + ci95,
            ciLower: meanTemp - ci95
        };
    });
}

function updateCharts(labelsMinuto, dataMinuto, labelsOra, dataOra, ciUpper, ciLower) {
    const minutoCtx = document.getElementById('minutoChart').getContext('2d');
    const oraCtx = document.getElementById('oraChart').getContext('2d');

    // Primo grafico (minuto per minuto)
    window.minutoChart = new Chart(minutoCtx, {
        type: 'line',
        data: {
            labels: labelsMinuto,
            datasets: [{
                label: 'Temperatura',
                data: dataMinuto,
                borderColor: 'rgba(75, 192, 192, 1)',
                fill: false,
                tension: 0.1
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false, // Permette una maggiore flessibilità
			plugins: {
                legend: {
                    display: false // Disabilita la leggenda
                }
            },
            scales: {
                x: {
                    display: true,
                    title: { display: true, text: '' },
                    ticks: {
                        callback: function(value, index, ticks) {
                            // Mostra solo una etichetta ogni 10 per esempio
                            if (index % 10 === 0) {
                                const date = parseDateString(labelsMinuto[value]);
                                if (!isNaN(date.getTime())) {
                                    return date.toLocaleDateString('it-IT');
                                } else {
                                    console.error('Errore nella conversione della data:', labelsMinuto[value]);
                                    return ''; // Restituisce una stringa vuota in caso di errore
                                }
                            } else {
                                return ''; // Non mostra nulla per le altre etichette
                            }
                        }
                    }
                },
                y: {
                    display: true,
                    title: { display: true, text: 'Temperatura (°C)' }
                }
            }
        }
    });

    // Secondo grafico (media oraria con intervalli di confidenza)
    window.oraChart = new Chart(oraCtx, {
        type: 'line',
        data: {
            labels: labelsOra,
            datasets: [
                {
                    label: 'Temperatura Media Oraria',
                    data: dataOra,
                    borderColor: 'rgba(153, 102, 255, 1)',
                    fill: false,
                    tension: 0.1
                },
                {
                    label: 'Confidenza 95%',
                    data: ciUpper,
                    borderColor: 'rgba(255, 159, 64, 0.2)',
                    fill: '-1',
                    backgroundColor: 'rgba(255, 159, 64, 0.2)',
                    borderWidth: 1
                },
                {
                    label: 'Confidenza Inferiore',
                    data: ciLower,
                    borderColor: 'rgba(255, 159, 64, 0.2)',
                    fill: '-1',
                    backgroundColor: 'rgba(255, 159, 64, 0.2)',
                    borderWidth: 1
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false, // Mantiene il rapporto d'aspetto
            plugins: {
                legend: {
                    display: false // Disabilita la leggenda
                }
            },
            scales: {
                x: {
                    display: true,
                    title: { display: true, text: '' },
                    ticks: {
                        callback: function(value, index, ticks) {
                            // Mostra solo una etichetta ogni 10 per esempio
                            if (index % 3 === 0) {
                                const date = parseDateString(labelsMinuto[value]);
                                if (!isNaN(date.getTime())) {
                                    return date.toLocaleDateString('it-IT');
                                } else {
                                    console.error('Errore nella conversione della data:', labelsMinuto[value]);
                                    return ''; // Restituisce una stringa vuota in caso di errore
                                }
                            } else {
                                return ''; // Non mostra nulla per le altre etichette
                            }
                        }
                    }
                },
                y: {
                    display: true,
                    title: { display: true, text: 'Temperatura (°C)' }
                }
            }
        }
    });
}

function rebindEvents() {
    document.getElementById('update-data').addEventListener('click', function() {
        loadDataAndUpdateCharts();
    });

    document.getElementById('timeFilter').addEventListener('change', function() {
        loadDataAndUpdateCharts();
    });
}

// Call rebindEvents after any dynamic content update
rebindEvents();  // Initial binding


document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('update-data').addEventListener('click', function() {
        loadDataAndUpdateCharts();
    });

    document.getElementById('timeFilter').addEventListener('change', function() {
        loadDataAndUpdateCharts();
    });

    // Caricamento iniziale dei dati
    loadDataAndUpdateCharts();
});



// Caricamento iniziale
loadDataAndUpdateCharts();
