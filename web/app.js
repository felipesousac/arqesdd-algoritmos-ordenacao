(() => {
  'use strict';

  // DOM helpers and display labels ---------------------------------------

  const element = (id) => document.getElementById(id);
  const chartCanvas = element('chart');
  const chartContext = chartCanvas.getContext('2d');

  const algorithmNames = [
    'Bubble Sort',
    'Selection Sort',
    'Insertion Sort',
    'Quick Sort',
    'Merge Sort'
  ];

  const algorithmDescriptions = [
    'Percorre a lista repetidamente, trocando vizinhos fora de ordem.',
    'Encontra o menor elemento restante e o coloca na próxima posição.',
    'Insere cada elemento em sua posição dentro da parte já ordenada.',
    'Divide a lista em torno de um pivô e ordena cada lado recursivamente.',
    'Divide a lista ao meio e combina as partes já ordenadas.'
  ];

  const colors = {
    bar: '#40d7ce',
    sortedBar: '#c7e66b',
    highlightedBar: '#ff9c61'
  };

  // These numeric values are shared with enum OperationType in the C engine.
  const OperationType = {
    COMPARE: 0,
    SWAP: 1,
    WRITE: 2
  };

  // Current engine and animation state -----------------------------------

  let sortingEngine = null;
  let initialValues = [];
  let animationFrames = [];
  let currentFrameIndex = 0;
  let playbackTimer = null;
  let displayedValues = [];
  let highlightedIndices = [];
  let sortedFromIndex = Infinity;

  // Engine loading and status --------------------------------------------

  async function loadSortingEngine() {
    try {
      const engineModule = await import('./sort_engine.js');
      sortingEngine = await engineModule.default();

      setEngineStatus('Motor C / WebAssembly ativo', true);
      element('sortButton').disabled = false;
    } catch (error) {
      sortingEngine = null;

      setEngineStatus('Motor C / WebAssembly indisponível', false);
      element('sortButton').disabled = true;
      element('sequenceError').textContent =
        'Não foi possível carregar o motor C/WebAssembly. Compile o projeto com Emscripten e recarregue a página.';
      element('sequenceError').hidden = false;

      console.error('Falha ao carregar o motor C/WebAssembly:', error);
    }

    applySequence();
  }

  function setEngineStatus(message, isReady) {
    element('engineStatus').textContent = message;
    element('statusDot').classList.toggle('ready', isReady);
  }

  // Input parsing and sequence setup -------------------------------------

  function readSequenceFromInput() {
    const rawInput = element('sequenceInput').value.trim();
    const tokens = rawInput ? rawInput.split(/[\s,;]+/).filter(Boolean) : [];
    const values = tokens.map(Number);

    let errorMessage = '';
    if (values.length < 2) {
      errorMessage = 'Informe pelo menos 2 valores.';
    } else if (values.length > 128) {
      errorMessage = 'O limite é de 128 valores para manter a animação fluida.';
    } else if (
      values.some(
        (value) =>
          !Number.isSafeInteger(value) || value < 1 || value > 1000000
      )
    ) {
      errorMessage = 'Use apenas inteiros positivos entre 1 e 1.000.000.';
    }

    element('sequenceError').textContent = errorMessage;
    element('sequenceError').hidden = !errorMessage;
    element('sequenceInput').setAttribute(
      'aria-invalid',
      String(Boolean(errorMessage))
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
    const itemCount = Number(element('sequenceCount').value);
    const errorMessage =
      !Number.isInteger(itemCount) || itemCount < 2 || itemCount > 128
        ? 'Informe uma quantidade inteira entre 2 e 128.'
        : '';

    element('sequenceError').textContent = errorMessage;
    element('sequenceError').hidden = !errorMessage;
    if (errorMessage) {
      return;
    }

    const values = Array.from({ length: itemCount }, (_, index) =>
      direction > 0 ? index + 1 : itemCount - index
    );

    element('sequenceInput').value = values.join(', ');
    element('sequenceInput').setAttribute('aria-invalid', 'false');
    initialValues = values;
    resetVisualization();
  }

  // Running C through WebAssembly ----------------------------------------

  function createFrame(operation, values, sortedFrom) {
    return {
      operation,
      values: values.slice(),
      sortedFrom
    };
  }

  function createAnimationFromWasm() {
    const operationBytes = 20;
    const inputPointer = sortingEngine._malloc(initialValues.length * 4);
    if (!inputPointer) {
      return null;
    }

    sortingEngine.HEAP32.set(initialValues, inputPointer >> 2);
    const sortSucceeded = sortingEngine._sort_run(
      inputPointer,
      initialValues.length,
      Number(element('algorithm').value),
      2000000
    );
    sortingEngine._free(inputPointer);

    if (!sortSucceeded) {
      return null;
    }

    const metricsPointer = sortingEngine._sort_metrics();
    const operationsPointer = sortingEngine._sort_operations();

    const itemCount = sortingEngine.HEAP32[(metricsPointer + 4) >> 2];
    const comparisons = readInt64(metricsPointer + 8);
    const swaps = readInt64(metricsPointer + 16);
    const elapsedMs = sortingEngine.HEAPF64[(metricsPointer + 24) >> 3];
    const operationCount = sortingEngine.HEAP32[(metricsPointer + 32) >> 2];

    const sortedValuesPointer = sortingEngine._sort_values();
    const sortedValues = Array.from(
      sortingEngine.HEAP32.subarray(
        sortedValuesPointer >> 2,
        (sortedValuesPointer >> 2) + itemCount
      )
    );

    const frames = [createFrame(null, initialValues, Infinity)];
    const valuesAtFrame = initialValues.slice();
    let comparisonCount = 0;
    let swapAndMovementCount = 0;

    for (let operationIndex = 0; operationIndex < operationCount; operationIndex++) {
      const operationPointer =
        operationsPointer + operationIndex * operationBytes;
      const operationType = sortingEngine.HEAP32[operationPointer >> 2];
      const firstIndex = sortingEngine.HEAP32[(operationPointer + 4) >> 2];
      const secondIndex = sortingEngine.HEAP32[(operationPointer + 8) >> 2];

      if (operationType === OperationType.COMPARE) {
        comparisonCount++;
      } else if (operationType === OperationType.SWAP) {
        swapAndMovementCount++;
        [valuesAtFrame[firstIndex], valuesAtFrame[secondIndex]] = [
          valuesAtFrame[secondIndex],
          valuesAtFrame[firstIndex]
        ];
      } else if (operationType === OperationType.WRITE) {
        valuesAtFrame[firstIndex] =
          sortingEngine.HEAP32[(operationPointer + 12) >> 2];

        const countsAsMovement =
          sortingEngine.HEAP32[(operationPointer + 16) >> 2];
        if (countsAsMovement) {
          swapAndMovementCount++;
        }
      }

      const frame = createFrame(
        { type: operationType, i: firstIndex, j: secondIndex },
        valuesAtFrame,
        Infinity
      );
      frame.comparisons = comparisonCount;
      frame.swaps = swapAndMovementCount;
      frame.time = (elapsedMs * operationIndex) / Math.max(1, operationCount);
      frames.push(frame);
    }

    const finalFrame = createFrame(null, sortedValues, 0);
    finalFrame.comparisons = comparisons;
    finalFrame.swaps = swaps;
    finalFrame.time = elapsedMs;

    const lastFrame = frames[frames.length - 1];
    if (frames.length === 1 || !sameValues(lastFrame.values, sortedValues)) {
      frames.push(finalFrame);
    } else {
      frames[frames.length - 1] = finalFrame;
    }

    sortingEngine._sort_free();
    return frames;
  }

  function readInt64(pointer) {
    const lowWord = sortingEngine.HEAP32[pointer >> 2];
    const highWord = sortingEngine.HEAP32[(pointer + 4) >> 2];
    return Number(lowWord) + Number(highWord) * 4294967296;
  }

  function sameValues(first, second) {
    return first.join(',') === second.join(',');
  }

  function startSorting() {
    if (!sortingEngine) {
      element('sequenceError').textContent =
        'O motor C/WebAssembly não está disponível. Compile o projeto com Emscripten e recarregue a página.';
      element('sequenceError').hidden = false;
      return;
    }

    stopPlayback();

    try {
      animationFrames = createAnimationFromWasm();
    } catch (error) {
      animationFrames = null;
      console.error('Falha ao executar o motor C/WebAssembly:', error);
    }

    if (!animationFrames) {
      element('sequenceError').textContent =
        'O motor C/WebAssembly não conseguiu executar a ordenação.';
      element('sequenceError').hidden = false;
      return;
    }

    currentFrameIndex = 0;
    element('timeline').max = Math.max(0, animationFrames.length - 1);
    element('timelineEnd').textContent = `${animationFrames.length - 1} PASSOS`;
    element('emptyState').hidden = true;
    element('stepLabel').textContent =
      `${algorithmNames[Number(element('algorithm').value)]} · ${animationFrames.length - 1} passos`;

    showFrame(0);
    playPlayback();
  }

  // Frame display and canvas rendering -----------------------------------

  function showFrame(frameIndex) {
    if (!animationFrames.length) {
      return;
    }

    currentFrameIndex = Math.max(
      0,
      Math.min(animationFrames.length - 1, frameIndex)
    );

    const frame = animationFrames[currentFrameIndex];
    displayedValues = frame.values;
    highlightedIndices = frame.operation
      ? [frame.operation.i, frame.operation.j].filter((index) => index >= 0)
      : [];
    sortedFromIndex = frame.sortedFrom;

    renderMetrics(frame.comparisons || 0, frame.swaps || 0, frame.time || 0);
    element('timeline').value = currentFrameIndex;

    const progress =
      currentFrameIndex / Math.max(1, animationFrames.length - 1);
    const progressPercent = Math.round(progress * 100);
    element('progress').innerHTML =
      `${progressPercent}<span class="percent">%</span>`;
    element('progressBar').style.width = `${progress * 100}%`;

    element('stepLabel').textContent = frame.operation
      ? `${operationName(frame.operation.type)} · passo ${currentFrameIndex} de ${animationFrames.length - 1}`
      : 'Vetor final';

    drawChart();
  }

  function operationName(operationType) {
    if (operationType === OperationType.COMPARE) {
      return 'Comparação';
    }
    if (operationType === OperationType.SWAP) {
      return 'Troca';
    }
    return 'Escrita';
  }

  function renderMetrics(comparisons, swaps, elapsedMs = 0) {
    element('comparisons').textContent = Number(comparisons).toLocaleString('pt-BR');
    element('swaps').textContent = Number(swaps).toLocaleString('pt-BR');
    element('time').textContent = Number(elapsedMs).toFixed(2);
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
        displayedValues.length
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
        0
      ]);
      chartContext.fill();
    });
  }

  function resetVisualization() {
    stopPlayback();

    displayedValues = initialValues.slice();
    animationFrames = [];
    currentFrameIndex = 0;
    highlightedIndices = [];
    sortedFromIndex = Infinity;

    renderMetrics(0, 0);
    drawChart();

    element('stepLabel').textContent = 'Vetor pronto';
    element('emptyState').hidden = true;
    element('timeline').max = 0;
    element('timeline').value = 0;
  }

  // Playback controls -----------------------------------------------------

  function stopPlayback() {
    if (playbackTimer) {
      clearInterval(playbackTimer);
      playbackTimer = null;
    }

    element('playButton').textContent = '▶';
    element('playButton').setAttribute('aria-label', 'Reproduzir');
  }

  function playPlayback() {
    if (
      !animationFrames.length ||
      currentFrameIndex >= animationFrames.length - 1
    ) {
      stopPlayback();
      return;
    }

    stopPlayback();
    element('playButton').textContent = 'Ⅱ';
    element('playButton').setAttribute('aria-label', 'Pausar');

    const speed = Number(element('speed').value);
    const intervalMs = Math.max(12, 180 - speed * 1.55);

    playbackTimer = setInterval(() => {
      if (currentFrameIndex >= animationFrames.length - 1) {
        stopPlayback();
        return;
      }

      showFrame(currentFrameIndex + 1);
    }, intervalMs);
  }

  // User interface event handlers ----------------------------------------

  element('sortButton').addEventListener('click', () => {
    const values = readSequenceFromInput();
    if (!values) {
      return;
    }

    initialValues = values;
    startSorting();
  });

  element('applySequence').addEventListener('click', applySequence);
  element('ascendingSequence').addEventListener('click', () =>
    applyOrderedSequence(1)
  );
  element('descendingSequence').addEventListener('click', () =>
    applyOrderedSequence(-1)
  );
  element('shuffleButton').addEventListener('click', applySequence);

  element('sequenceInput').addEventListener('keydown', (event) => {
    if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') {
      applySequence();
    }
  });

  element('resetButton').addEventListener('click', () => {
    stopPlayback();
    showFrame(0);
  });

  element('playButton').addEventListener('click', () => {
    if (playbackTimer) {
      stopPlayback();
    } else {
      playPlayback();
    }
  });

  element('stepBack').addEventListener('click', () => {
    stopPlayback();
    showFrame(currentFrameIndex - 1);
  });

  element('stepForward').addEventListener('click', () => {
    stopPlayback();
    showFrame(currentFrameIndex + 1);
  });

  element('timeline').addEventListener('input', (event) => {
    stopPlayback();
    showFrame(Number(event.target.value));
  });

  element('algorithm').addEventListener('change', (event) => {
    element('algorithmNote').textContent =
      algorithmDescriptions[Number(event.target.value)];
    resetVisualization();
  });

  window.addEventListener('resize', drawChart);

  loadSortingEngine();
})();
