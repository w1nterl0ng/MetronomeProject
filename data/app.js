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
            <div class="patch" draggable="true" data-index="${index}">
                <span class="patch-handle material-icons">drag_indicator</span>
                <input type="text" value="${patch.name}" maxlength="4" 
                       pattern="[A-Za-z0-9 ]{1,4}"
                       onchange="updatePatch(${index}, 'name', this.value)">
                <input type="number" value="${patch.tempo}" min="40" max="240"
                       onchange="updatePatch(${index}, 'tempo', this.value)">
                <button class="delete-btn" onclick="deletePatch(${index})">Delete</button>
            </div>
        `).join('');

    setupDragAndDrop();
}

async function createPatch() {
    const nameInput = document.getElementById('new-patch-name');
    const tempoInput = document.getElementById('new-patch-tempo');
    const name = nameInput.value;
    const tempo = parseInt(tempoInput.value);

    if (!name || !tempo || name.length > 4 || tempo < 40 || tempo > 240) {
        showMessage('Please enter a valid name (1-4 chars) and tempo (40-240)', 'error');
        return;
    }

    try {
        const response = await fetch('/api/patches', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({ name, tempo })
        });

        if (response.ok) {
            await loadPatches();
            nameInput.value = '';
            tempoInput.value = '';
            showMessage('Patch created', 'success');
        }
    } catch (error) {
        showMessage('Error creating patch: ' + error.message, 'error');
    }
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

async function deletePatch(index) {
    if (!confirm('Are you sure you want to delete this patch?')) return;

    try {
        const response = await fetch('/api/patches', {
            method: 'DELETE',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({ index })
        });

        if (response.ok) {
            await loadPatches();
            showMessage('Patch deleted', 'success');
        }
    } catch (error) {
        showMessage('Error deleting patch: ' + error.message, 'error');
    }
}

function setupDragAndDrop() {
    const patchElements = document.querySelectorAll('.patch');

    patchElements.forEach(patch => {
        patch.addEventListener('dragstart', e => {
            patch.classList.add('dragging');
            e.dataTransfer.setData('text/plain', patch.dataset.index);
        });

        patch.addEventListener('dragend', () => {
            patch.classList.remove('dragging');
        });

        patch.addEventListener('dragover', e => {
            e.preventDefault();
        });

        patch.addEventListener('drop', async e => {
            e.preventDefault();
            const fromIndex = parseInt(e.dataTransfer.getData('text/plain'));
            const toIndex = parseInt(patch.dataset.index);

            if (fromIndex !== toIndex) {
                const [movedPatch] = patches.splice(fromIndex, 1);
                patches.splice(toIndex, 0, movedPatch);

                try {
                    const response = await fetch('/api/patches/reorder', {
                        method: 'POST',
                        headers: {'Content-Type': 'application/json'},
                        body: JSON.stringify({ patches })
                    });
                    
                    if (response.ok) {
                        renderPatches();
                        showMessage('Patches reordered', 'success');
                    }
                } catch (error) {
                    showMessage('Error reordering patches: ' + error.message, 'error');
                    loadPatches();
                }
            }
        });
    });
}

async function loadSettings() {
    try {
        const response = await fetch('/api/settings');
        const settings = await response.json();
        document.getElementById('brightness').value = settings.brightness;
        document.getElementById('brightness-value').textContent = settings.brightness;
    } catch (error) {
        showMessage('Error loading settings: ' + error.message, 'error');
    }
}

async function saveSettings() {
    const settings = {
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