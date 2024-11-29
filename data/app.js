let patches = [];

async function loadPatches() {
    try {
        const response = await fetch('/api/patches');
        patches = await response.json();
        renderPatches();
    } catch (error) {
        showMessage('Error loading patches: ' + error.message, 'error');
    }
}

function renderPatches() {
    const patchesList = document.getElementById('patches-list');
    patchesList.innerHTML = patches
        .filter(patch => patch.name !== '')
        .map((patch, index) => `
            <div class="patch">
                <input type="text" value="${patch.name}" maxlength="4"
                       onchange="updatePatch(${index}, 'name', this.value)">
                <input type="number" value="${patch.tempo}" min="40" max="240"
                       onchange="updatePatch(${index}, 'tempo', this.value)">
            </div>
        `).join('');
}

async function updatePatch(index, field, value) {
    const patch = patches[index];
    patch[field] = field === 'tempo' ? parseInt(value) : value;

    try {
        const response = await fetch('/api/patches', {
            method: 'PUT',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({ index, patch })
        });

        if (response.ok) {
            showMessage('Patch updated', 'success');
        }
    } catch (error) {
        showMessage('Error updating patch: ' + error.message, 'error');
        loadPatches();
    }
}

async function loadSettings() {
    try {
        const response = await fetch('/api/settings');
        const settings = await response.json();
        document.getElementById('live-gig-mode').checked = settings.liveGigMode;
        document.getElementById('brightness').value = settings.brightness;
        document.getElementById('brightness-value').textContent = settings.brightness;
    } catch (error) {
        showMessage('Error loading settings: ' + error.message, 'error');
    }
}

async function saveSettings() {
    const settings = {
        liveGigMode: document.getElementById('live-gig-mode').checked,
        brightness: parseInt(document.getElementById('brightness').value)
    };

    try {
        const response = await fetch('/api/settings', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(settings)
        });

        if (response.ok) {
            showMessage('Settings saved', 'success');
        }
    } catch (error) {
        showMessage('Error saving settings: ' + error.message, 'error');
    }
}

function showMessage(text, type) {
    const messageEl = document.getElementById('message');
    messageEl.textContent = text;
    messageEl.className = type;
    setTimeout(() => {
        messageEl.textContent = '';
        messageEl.className = '';
    }, 3000);
}

document.getElementById('brightness').addEventListener('input', function() {
    document.getElementById('brightness-value').textContent = this.value;
});

// Initial load
loadPatches();
loadSettings();