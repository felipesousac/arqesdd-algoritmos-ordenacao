(() => {
  "use strict";

  // Dynamic Linked Stack TAD (Undo / Redo Step-by-Step History) -----------
  class StackNode {
    constructor(data, next = null) {
      this.data = data;
      this.next = next;
    }
  }

  class OperationStack {
    constructor() {
      this.top = null;
      this.count = 0;
    }

    push(element) {
      this.top = new StackNode(element, this.top);
      this.count++;
    }

    pop() {
      if (this.top === null) {
        return null;
      }

      const removed = this.top;

      this.top = removed.next;

      this.count--;

      return removed.data;
    }

    peek() {
      return this.top !== null ? this.top.data : null;
    }

    isEmpty() {
      return this.top === null;
    }

    size() {
      return this.count;
    }

    clear() {
      this.top = null;
      this.count = 0;
    }
  }

  // DOM helpers and display labels ---------------------------------------
  const element = (id) => document.getElementById(id);
  const chartCanvas = element("chart");
  const chartContext = chartCanvas.getContext("2d");

  const algorithmNames = [
    "Bubble Sort",
    "Selection Sort",
    "Insertion Sort",
    "Quick Sort",
    "Merge Sort",
  ];

  const algorithmDescriptions = [
    "Percorre a lista repetidamente, trocando vizinhos fora de ordem.",
    "Encontra o menor elemento restante e o coloca na próxima posição.",
    "Insere cada elemento em sua posição dentro da parte já ordenada.",
    "Divide a lista em torno de um pivô e ordena cada lado recursivamente.",
    "Divide a lista ao meio e combina as partes já ordenadas.",
  ];

  const structureNames = [
    "Lista Sequencial (Vetor Dinâmico)",
    "Lista Simplesmente Encadeada",
    "Lista Duplamente Encadeada",
    "Lista Circular Encadeada",
  ];

  const structureDescriptions = [
    "Bloco contíguo no heap alocado com malloc/realloc; acesso O(1) por índice.",
    "Cadeia dinâmica de nós (dado + ponteiro next); ordenação religando ponteiros.",
    "Cadeia dinâmica de nós (dado + ponteiros prev/next); percurso bidirecional.",
    "Anel dinâmico de nós onde tail->next aponta de volta para head.",
  ];

  const colors = {
    bar: "#40d7ce",
    sortedBar: "#c7e66b",
    highlightedBar: "#ff9c61",
  };

  // Shared with enum OperationType in the C engine
  const OperationType = {
    COMPARE: 0,
    SWAP: 1,
    WRITE: 2,
  };

  // Current engine, animation, and Stack TAD state -----------------------

  let initialValues = [];
  let rawOperations = [];
  let currentHistory = null;
  let totalSteps = 0;
  let currentStepIndex = 0;
  let playbackTimer = null;
  let displayedValues = [];
  let highlightedIndices = [];
  let sortedFromIndex = Infinity;
  let currentComparisons = 0;
  let currentSwaps = 0;
  let currentTimeMs = 0;
  let currentMemoryBytes = 0;

  const undoStack = new OperationStack();
  const redoStack = new OperationStack();

  function translateStructureName(structureId, fallbackName) {
    return structureNames[Number(structureId)] || fallbackName || "";
  }

  // API initialization ----------------------------------------------------
  function loadSortingEngine() {
    setEngineStatus("API C pronta", true);
    element("sortButton").disabled = false;
    applySequence();
    refreshHashHistory();
  }

  function setEngineStatus(message, isReady) {
    element("engineStatus").textContent = message;
    element("statusDot").classList.toggle("ready", isReady);
  }

  // Input parsing and sequence setup -------------------------------------
  function readSequenceFromInput() {
    const rawInput = element("sequenceInput").value.trim();
    const tokens = rawInput ? rawInput.split(/[\s,;]+/).filter(Boolean) : [];
    const values = tokens.map(Number);

    let errorMessage = "";

    if (values.length < 2) {
      errorMessage = "Informe pelo menos 2 valores.";
    } else if (values.length > 128) {
      errorMessage = "O limite é de 128 valores para manter a animação fluida.";
    } else if (
      values.some(
        (value) => !Number.isSafeInteger(value) || value < 1 || value > 1000000,
      )
    ) {
      errorMessage = "Use apenas inteiros positivos entre 1 e 1.000.000.";
    }

    element("sequenceError").textContent = errorMessage;
    element("sequenceError").hidden = !errorMessage;
    element("sequenceInput").setAttribute(
      "aria-invalid",
      String(Boolean(errorMessage)),
    );

    return errorMessage ? null : values;
  }

  function applySequence() {
    const values = readSequenceFromInput();

    if (!values) {
      return;
    }

    initialValues = values;
    resetVisualization();
  }

  function applyOrderedSequence(direction) {
    const itemCount = Number(element("sequenceCount").value);

    const errorMessage =
      !Number.isInteger(itemCount) || itemCount < 2 || itemCount > 128
        ? "Informe uma quantidade inteira entre 2 e 128."
        : "";

    element("sequenceError").textContent = errorMessage;
    element("sequenceError").hidden = !errorMessage;

    if (errorMessage) {
      return;
    }

    const values = Array.from({ length: itemCount }, (_, index) =>
      direction > 0 ? index + 1 : itemCount - index,
    );

    element("sequenceInput").value = values.join(", ");
    element("sequenceInput").setAttribute("aria-invalid", "false");
    initialValues = values;
    resetVisualization();
  }

  // Reversible Stack-based Step Execution (Undo / Redo) ------------------
  function loadHistoryIntoPlayer(history) {
    currentHistory = history;
    initialValues = (history.input || []).slice();
    rawOperations = (history.operations || []).slice();
    totalSteps = rawOperations.length + 1;
    currentMemoryBytes =
      history.metrics && history.metrics.memoryBytes
        ? history.metrics.memoryBytes
        : 0;

    undoStack.clear();
    redoStack.clear();

    displayedValues = initialValues.slice();
    highlightedIndices = [];
    sortedFromIndex = Infinity;
    currentStepIndex = 0;
    currentComparisons = 0;
    currentSwaps = 0;
    currentTimeMs = 0;

    element("timeline").max = Math.max(0, totalSteps);
    element("timeline").value = 0;
    element("timelineEnd").textContent = `${totalSteps} PASSOS`;
    element("emptyState").hidden = true;

    updateStackUI();
    renderState();
  }

  // Advances one step forward using RedoStack or next raw operation, pushing a reversible record onto UndoStack
  function stepForwardOne() {
    if (!currentHistory || currentStepIndex >= totalSteps) {
      return false;
    }

    const redoRecord = !redoStack.isEmpty() ? redoStack.pop() : null;
    const stepIdx = currentStepIndex + 1;

    if (stepIdx <= rawOperations.length) {
      const op = redoRecord ? redoRecord.operation : rawOperations[stepIdx - 1];
      const reversibleRecord = {
        stepIndex: stepIdx,
        isFinalStep: false,
        operation: op,
        prevFirstValue:
          op.firstIndex >= 0 ? displayedValues[op.firstIndex] : null,
        prevSecondValue:
          op.secondIndex >= 0 ? displayedValues[op.secondIndex] : null,
        prevComparisons: currentComparisons,
        prevSwaps: currentSwaps,
        prevTimeMs: currentTimeMs,
        prevHighlighted: highlightedIndices.slice(),
        prevSortedFrom: sortedFromIndex,
      };

      if (op.type === OperationType.COMPARE) {
        currentComparisons++;
      } else if (op.type === OperationType.SWAP) {
        currentSwaps++;
        const temp = displayedValues[op.firstIndex];
        displayedValues[op.firstIndex] = displayedValues[op.secondIndex];
        displayedValues[op.secondIndex] = temp;
      } else if (op.type === OperationType.WRITE) {
        displayedValues[op.firstIndex] = op.firstValue;
        if (op.secondValue) {
          currentSwaps++;
        }
      }

      currentTimeMs =
        (currentHistory.metrics.elapsedMs * stepIdx) /
        Math.max(1, rawOperations.length);
      highlightedIndices = [op.firstIndex, op.secondIndex].filter(
        (idx) => idx >= 0,
      );
      sortedFromIndex = Infinity;

      undoStack.push(reversibleRecord);
      currentStepIndex = stepIdx;
    } else {
      // Final completion step
      const reversibleRecord = {
        stepIndex: stepIdx,
        isFinalStep: true,
        operation: null,
        prevValuesSnapshot: displayedValues.slice(),
        prevComparisons: currentComparisons,
        prevSwaps: currentSwaps,
        prevTimeMs: currentTimeMs,
        prevHighlighted: highlightedIndices.slice(),
        prevSortedFrom: sortedFromIndex,
      };

      displayedValues = (currentHistory.sorted || displayedValues).slice();
      currentComparisons = currentHistory.metrics.comparisons;
      currentSwaps = currentHistory.metrics.swaps;
      currentTimeMs = currentHistory.metrics.elapsedMs;
      highlightedIndices = [];
      sortedFromIndex = 0;

      undoStack.push(reversibleRecord);
      currentStepIndex = stepIdx;
    }

    return true;
  }

  // Steps backward by popping from UndoStack, surgically reverting displayedValues, and pushing onto RedoStack
  function stepBackwardOne() {
    if (undoStack.isEmpty() || currentStepIndex <= 0) {
      return false;
    }

    const record = undoStack.pop();
    redoStack.push(record);

    if (record.isFinalStep) {
      displayedValues = record.prevValuesSnapshot.slice();
    } else {
      const op = record.operation;
      if (op.type === OperationType.SWAP) {
        displayedValues[op.firstIndex] = record.prevFirstValue;
        displayedValues[op.secondIndex] = record.prevSecondValue;
      } else if (op.type === OperationType.WRITE) {
        displayedValues[op.firstIndex] = record.prevFirstValue;
      }
    }

    currentComparisons = record.prevComparisons;
    currentSwaps = record.prevSwaps;
    currentTimeMs = record.prevTimeMs;
    highlightedIndices = record.prevHighlighted.slice();
    sortedFromIndex = record.prevSortedFrom;
    currentStepIndex = record.stepIndex - 1;

    return true;
  }

  function seekToStep(targetStep) {
    if (!currentHistory) {
      return;
    }
    const clampedTarget = Math.max(0, Math.min(totalSteps, targetStep));
    while (currentStepIndex < clampedTarget) {
      if (!stepForwardOne()) {
        break;
      }
    }
    while (currentStepIndex > clampedTarget) {
      if (!stepBackwardOne()) {
        break;
      }
    }
    updateStackUI();
    renderState();
  }

  function updateStackUI() {
    element("undoStackSize").textContent = String(undoStack.size());
    element("redoStackSize").textContent = String(redoStack.size());

    const topRecord = undoStack.peek();

    if (!topRecord) {
      element("stackTopLabel").textContent =
        "Topo da pilha: vazia (estado inicial)";
    } else if (topRecord.isFinalStep) {
      element("stackTopLabel").textContent =
        `Topo da pilha: #${topRecord.stepIndex} CONCLUÍDO`;
    } else {
      const op = topRecord.operation;

      const opLabel = operationName(op.type).toUpperCase();

      element("stackTopLabel").textContent =
        op.secondIndex >= 0 && op.type !== OperationType.WRITE
          ? `Topo da pilha: #${topRecord.stepIndex} ${opLabel}(${op.firstIndex}, ${op.secondIndex})`
          : `Topo da pilha: #${topRecord.stepIndex} ${opLabel}[${op.firstIndex}] = ${op.firstValue}`;
    }
  }

  // Rendering state, metrics, and chart ----------------------------------
  function renderState() {
    renderMetrics(
      currentComparisons,
      currentSwaps,
      currentTimeMs,
      currentMemoryBytes,
    );

    element("timeline").value = currentStepIndex;

    const progress =
      totalSteps > 0 ? currentStepIndex / Math.max(1, totalSteps) : 0;

    const progressPercent = Math.round(progress * 100);

    element("progress").innerHTML =
      `${progressPercent}<span class="percent">%</span>`;
    element("progressBar").style.width = `${progress * 100}%`;

    const topRecord = undoStack.peek();

    if (!currentHistory) {
      element("stepLabel").textContent = "Vetor pronto";
    } else {
      const structLabel = translateStructureName(
        currentHistory.structure,
        currentHistory.structureName,
      );

      if (!topRecord) {
        element("stepLabel").textContent =
          `${currentHistory.algorithmName} em ${structLabel} · passo 0 de ${totalSteps}`;
      } else if (topRecord.isFinalStep) {
        element("stepLabel").textContent =
          `Estrutura ordenada (${structLabel}) · ${totalSteps} passos`;
      } else {
        element("stepLabel").textContent =
          `${operationName(topRecord.operation.type)} · passo ${currentStepIndex} de ${totalSteps} (Pilha: ${undoStack.size()})`;
      }
    }

    drawChart();
  }

  function operationName(operationType) {
    if (operationType === OperationType.COMPARE) {
      return "Comparação";
    }
    if (operationType === OperationType.SWAP) {
      return "Troca";
    }
    return "Escrita";
  }

  function formatMemoryBytes(bytes) {
    const num = Number(bytes) || 0;
    if (num >= 1024) {
      return `${(num / 1024).toFixed(2)} KB (${num} B)`;
    }
    return `${num} B`;
  }

  function renderMetrics(comparisons, swaps, elapsedMs = 0, memoryBytes = 0) {
    element("comparisons").textContent =
      Number(comparisons).toLocaleString("pt-BR");
    element("swaps").textContent = Number(swaps).toLocaleString("pt-BR");
    element("time").textContent = Number(elapsedMs).toFixed(3);
    element("memory").textContent = Number(memoryBytes).toLocaleString("pt-BR");
  }

  function drawChart() {
    const devicePixelRatio = window.devicePixelRatio || 1;
    const bounds = chartCanvas.getBoundingClientRect();

    chartCanvas.width = Math.round(bounds.width * devicePixelRatio);
    chartCanvas.height = Math.round(bounds.height * devicePixelRatio);
    chartContext.setTransform(devicePixelRatio, 0, 0, devicePixelRatio, 0, 0);
    chartContext.clearRect(0, 0, bounds.width, bounds.height);

    if (!displayedValues.length) {
      return;
    }

    const gap = displayedValues.length > 90 ? 1 : 2;

    const barWidth = Math.max(
      1,
      (bounds.width - gap * (displayedValues.length - 1)) /
        displayedValues.length,
    );
    const largestValue = Math.max(...displayedValues);
    const chartHeight = bounds.height - 8;

    displayedValues.forEach((value, index) => {
      const barHeight = Math.max(2, (value / largestValue) * chartHeight);
      const x = index * (barWidth + gap);
      const y = bounds.height - barHeight;

      if (highlightedIndices.includes(index)) {
        chartContext.fillStyle = colors.highlightedBar;
      } else if (index >= sortedFromIndex) {
        chartContext.fillStyle = colors.sortedBar;
      } else {
        chartContext.fillStyle = colors.bar;
      }

      const cornerRadius = Math.min(3, barWidth / 2);

      chartContext.beginPath();

      chartContext.roundRect(x, y, barWidth, barHeight, [
        cornerRadius,
        cornerRadius,
        0,
        0,
      ]);

      chartContext.fill();
    });
  }

  function resetVisualization() {
    stopPlayback();

    currentHistory = null;
    rawOperations = [];
    totalSteps = 0;
    currentStepIndex = 0;
    displayedValues = initialValues.slice();
    highlightedIndices = [];
    sortedFromIndex = Infinity;
    currentComparisons = 0;
    currentSwaps = 0;
    currentTimeMs = 0;
    currentMemoryBytes = 0;

    undoStack.clear();
    redoStack.clear();

    updateStackUI();
    renderMetrics(0, 0, 0, 0);
    drawChart();

    element("stepLabel").textContent = "Vetor pronto";
    element("emptyState").hidden = true;
    element("timeline").max = 0;
    element("timeline").value = 0;
  }

  // API Calls: Single Sort, Batch Queue, and Hash Table History ----------
  async function startSorting() {
    const values = readSequenceFromInput();

    if (!values) {
      return;
    }

    initialValues = values;
    stopPlayback();
    element("sortButton").disabled = true;

    try {
      const query = new URLSearchParams({
        algorithm: element("algorithm").value,
        structure: element("structure").value,
        values: values.join(","),
      });

      const response = await fetch(`/api/sort?${query}`);

      const history = await response.json();

      if (!response.ok) {
        throw new Error(history.error || "Falha na API de ordenação.");
      }

      loadHistoryIntoPlayer(history);
      playPlayback();
      refreshHashHistory();
    } catch (error) {
      element("sequenceError").textContent =
        `Não foi possível obter o histórico da API C: ${error.message}`;
      element("sequenceError").hidden = false;
    } finally {
      element("sortButton").disabled = false;
    }
  }

  async function runBatchBenchmark(algorithmsParam, structuresParam) {
    const values = readSequenceFromInput();

    if (!values) {
      return;
    }

    element("batchStatusBadge").textContent = "Processando Fila em C…";

    try {
      const query = new URLSearchParams({
        algorithms: algorithmsParam,
        structures: structuresParam,
        values: values.join(","),
      });

      const response = await fetch(`/api/batch?${query}`);

      const data = await response.json();

      if (!response.ok) {
        throw new Error(data.error || "Falha na execução em lote.");
      }

      element("batchStatusBadge").textContent =
        `${data.queueTasksProcessed} tarefas desenfileiradas · Pico da Fila: ${data.queuePeakMemoryBytes} B`;
      renderBatchTable(data.results || []);
      refreshHashHistory();
    } catch (error) {
      element("batchStatusBadge").textContent = "Erro no lote";
      element("sequenceError").textContent =
        `Falha ao processar lote na Fila C: ${error.message}`;
      element("sequenceError").hidden = false;
    }
  }

  function renderBatchTable(results) {
    const tbody = element("batchTableBody");
    tbody.innerHTML = "";

    if (!results.length) {
      tbody.innerHTML =
        '<tr><td colspan="8" class="empty-row">Nenhum resultado retornado.</td></tr>';
      return;
    }

    results.forEach((item) => {
      const tr = document.createElement("tr");
      const structLabel = translateStructureName(
        item.structure,
        item.structureName,
      );

      tr.innerHTML = `
        <td>#${item.testId}</td>
        <td>${item.algorithmName}</td>
        <td>${structLabel}</td>
        <td>${Number(item.metrics.comparisons).toLocaleString("pt-BR")}</td>
        <td>${Number(item.metrics.swaps).toLocaleString("pt-BR")}</td>
        <td>${Number(item.metrics.elapsedMs).toFixed(3)}</td>
        <td>${formatMemoryBytes(item.metrics.memoryBytes)}</td>
        <td><button type="button" class="replay-btn" data-test-id="${item.testId}">Reproduzir</button></td>
      `;

      tbody.appendChild(tr);
    });
  }

  async function refreshHashHistory() {
    const filterValue = element("historyFilter").value;

    try {
      const query = new URLSearchParams({ algorithm: filterValue });
      const response = await fetch(`/api/history?${query}`);
      const data = await response.json();

      if (!response.ok) {
        return;
      }

      if (data.tableStats) {
        element("hashStatsBadge").textContent =
          `${data.tableStats.totalTests} testes · ${data.tableStats.bucketCount} buckets · ${formatMemoryBytes(data.tableStats.memoryBytes)}`;
      }

      renderHistoryTable(data.reports || []);
    } catch {
      // Ignored, keep current table if offline
    }
  }

  function renderHistoryTable(reports) {
    const tbody = element("historyTableBody");

    tbody.innerHTML = "";

    if (!reports.length) {
      tbody.innerHTML =
        '<tr><td colspan="9" class="empty-row">Nenhum teste encontrado neste bucket da Tabela Hash.</td></tr>';

      return;
    }

    const sortedReports = reports
      .slice()
      .sort((a, b) => Number(b.testId) - Number(a.testId));

    sortedReports.forEach((rep) => {
      const tr = document.createElement("tr");

      const structLabel = translateStructureName(
        rep.structure,
        rep.structureName,
      );

      tr.innerHTML = `
        <td>#${rep.testId}</td>
        <td>${rep.algorithmName}</td>
        <td>${structLabel}</td>
        <td>${rep.metrics.itemCount}</td>
        <td>${Number(rep.metrics.comparisons).toLocaleString("pt-BR")}</td>
        <td>${Number(rep.metrics.swaps).toLocaleString("pt-BR")}</td>
        <td>${Number(rep.metrics.elapsedMs).toFixed(3)}</td>
        <td>${formatMemoryBytes(rep.metrics.memoryBytes)}</td>
        <td><button type="button" class="replay-btn" data-test-id="${rep.testId}">Reproduzir</button></td>
      `;

      tbody.appendChild(tr);
    });
  }

  async function replayTestById(testId) {
    stopPlayback();

    try {
      const response = await fetch(
        `/api/history/detail?id=${encodeURIComponent(testId)}`,
      );

      const history = await response.json();

      if (!response.ok) {
        throw new Error(history.error || "Não foi possível carregar o teste.");
      }

      element("algorithm").value = String(history.algorithm);

      element("algorithmNote").textContent =
        algorithmDescriptions[Number(history.algorithm)] || "";

      element("structure").value = String(history.structure);

      element("structureNote").textContent =
        structureDescriptions[Number(history.structure)] || "";

      element("sequenceInput").value = (history.input || []).join(", ");

      loadHistoryIntoPlayer(history);
      window.scrollTo({ top: 0, behavior: "smooth" });
      playPlayback();
    } catch (error) {
      element("sequenceError").textContent =
        `Falha ao reproduzir teste #${testId}: ${error.message}`;
      element("sequenceError").hidden = false;
    }
  }

  // Playback controls -----------------------------------------------------
  function stopPlayback() {
    if (playbackTimer) {
      clearInterval(playbackTimer);
      playbackTimer = null;
    }

    element("playButton").textContent = "▶";
    element("playButton").setAttribute("aria-label", "Reproduzir");
  }

  function playPlayback() {
    if (!currentHistory || currentStepIndex >= totalSteps) {
      stopPlayback();

      return;
    }

    stopPlayback();
    element("playButton").textContent = "Ⅱ";
    element("playButton").setAttribute("aria-label", "Pausar");

    const speed = Number(element("speed").value);
    const intervalMs = Math.max(12, 180 - speed * 1.55);

    playbackTimer = setInterval(() => {
      if (!stepForwardOne()) {
        stopPlayback();

        return;
      }

      updateStackUI();
      renderState();

      if (currentStepIndex >= totalSteps) {
        stopPlayback();
      }
    }, intervalMs);
  }

  // User interface event handlers ----------------------------------------
  element("sortButton").addEventListener("click", startSorting);
  element("applySequence").addEventListener("click", applySequence);
  element("ascendingSequence").addEventListener("click", () =>
    applyOrderedSequence(1),
  );
  element("descendingSequence").addEventListener("click", () =>
    applyOrderedSequence(-1),
  );
  element("shuffleButton").addEventListener("click", applySequence);

  element("sequenceInput").addEventListener("keydown", (event) => {
    if ((event.ctrlKey || event.metaKey) && event.key === "Enter") {
      applySequence();
    }
  });

  element("resetButton").addEventListener("click", () => {
    stopPlayback();
    seekToStep(0);
  });

  element("playButton").addEventListener("click", () => {
    if (playbackTimer) {
      stopPlayback();
    } else {
      playPlayback();
    }
  });

  element("stepBack").addEventListener("click", () => {
    stopPlayback();
    if (stepBackwardOne()) {
      updateStackUI();
      renderState();
    }
  });

  element("stepForward").addEventListener("click", () => {
    stopPlayback();
    if (stepForwardOne()) {
      updateStackUI();
      renderState();
    }
  });

  element("timeline").addEventListener("input", (event) => {
    stopPlayback();
    seekToStep(Number(event.target.value));
  });

  element("algorithm").addEventListener("change", (event) => {
    element("algorithmNote").textContent =
      algorithmDescriptions[Number(event.target.value)];
    resetVisualization();
  });

  element("structure").addEventListener("change", (event) => {
    element("structureNote").textContent =
      structureDescriptions[Number(event.target.value)];
    resetVisualization();
  });

  element("batchCompareStructures").addEventListener("click", () => {
    runBatchBenchmark(element("algorithm").value, "0,1,2,3");
  });

  element("batchCompareAlgorithms").addEventListener("click", () => {
    runBatchBenchmark("0,1,2,3,4", element("structure").value);
  });

  element("batchFullMatrix").addEventListener("click", () => {
    runBatchBenchmark("0,1,2,3,4", "0,1,2,3");
  });

  function openClearHistoryModal() {
    element("clearHistoryModal").hidden = false;
  }

  function closeClearHistoryModal() {
    element("clearHistoryModal").hidden = true;
  }

  function resetBatchCard() {
    element("batchStatusBadge").textContent = "Fila aguardando";
    element("batchTableBody").innerHTML =
      '<tr><td colspan="8" class="empty-row">Selecione uma opção acima para enfileirar e comparar os cenários.</td></tr>';
  }

  async function confirmClearHashHistory() {
    try {
      const response = await fetch("/api/history/clear", { method: "DELETE" });

      if (!response.ok) {
        throw new Error("Não foi possível limpar o histórico na API C.");
      }

      closeClearHistoryModal();
      resetBatchCard();
      refreshHashHistory();
    } catch (error) {
      closeClearHistoryModal();
      element("sequenceError").textContent = error.message;
      element("sequenceError").hidden = false;
    }
  }

  element("historyFilter").addEventListener("change", refreshHashHistory);
  element("clearHistoryButton").addEventListener(
    "click",
    openClearHistoryModal,
  );
  element("cancelClearHistory").addEventListener(
    "click",
    closeClearHistoryModal,
  );
  element("confirmClearHistory").addEventListener(
    "click",
    confirmClearHashHistory,
  );

  element("clearHistoryModal").addEventListener("click", (event) => {
    if (event.target === element("clearHistoryModal")) {
      closeClearHistoryModal();
    }
  });

  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && !element("clearHistoryModal").hidden) {
      closeClearHistoryModal();
    }
  });

  document.addEventListener("click", (event) => {
    const replayButton = event.target.closest(".replay-btn");

    if (replayButton && replayButton.dataset.testId) {
      replayTestById(replayButton.dataset.testId);
    }
  });

  window.addEventListener("resize", drawChart);

  loadSortingEngine();
})();
