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

  let initialValues = [];
  let animationFrames = [];
  let currentFrameIndex = 0;
  let playbackTimer = null;
  let displayedValues = [];
  let highlightedIndices = [];
  let sortedFromIndex = Infinity;

  // API status ------------------------------------------------------------

  function loadSortingEngine() {
    setEngineStatus('API C pronta', true);
    element('sortButton').disabled = false;
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

  // Consume the sorting history returned by the C API --------------------

  function createFrame(operation, values, sortedFrom) {
    return {
      operation,
      values: values.slice(),
      sortedFrom
    };
  }

  function createAnimationFromHistory(history) {
    const frames = [createFrame(null, history.input, Infinity)];
    const valuesAtFrame = history.input.slice();
    let comparisonCount = 0;
    let swapAndMovementCount = 0;
    const operations = history.operations || [];

    operations.forEach((operation, operationIndex) => {
      if (operation.type === OperationType.COMPARE) {
        comparisonCount++;
      } else if (operation.type === OperationType.SWAP) {
        swapAndMovementCount++;
        [
          valuesAtFrame[operation.firstIndex],
          valuesAtFrame[operation.secondIndex]
        ] = [
          valuesAtFrame[operation.secondIndex],
          valuesAtFrame[operation.firstIndex]
        ];
      } else if (operation.type === OperationType.WRITE) {
        valuesAtFrame[operation.firstIndex] = operation.firstValue;
        if (operation.secondValue) {
          swapAndMovementCount++;
        }
      }

      const frame = createFrame(
        {
          type: operation.type,
          i: operation.firstIndex,
          j: operation.secondIndex
        },
        valuesAtFrame,
        Infinity
      );
      frame.comparisons = comparisonCount;
      frame.swaps = swapAndMovementCount;
      frame.time =
        (history.metrics.elapsedMs * (operationIndex + 1)) /
        Math.max(1, operations.length);
      frames.push(frame);
    });

    const finalFrame = createFrame(null, history.sorted, 0);
    finalFrame.comparisons = history.metrics.comparisons;
    finalFrame.swaps = history.metrics.swaps;
    finalFrame.time = history.metrics.elapsedMs;
    const lastFrame = frames[frames.length - 1];
    if (frames.length === 1 || !sameValues(lastFrame.values, history.sorted)) {
      frames.push(finalFrame);
    } else {
      frames[frames.length - 1] = finalFrame;
    }
    return frames;
  }

  function sameValues(first, second) {
    return first.join(',') === second.join(',');
  }

  async function startSorting() {
    const values = readSequenceFromInput();
    if (!values) {
      return;
    }

    initialValues = values;
    stopPlayback();
    element('sortButton').disabled = true;
    try {
      const query = new URLSearchParams({
        algorithm: element('algorithm').value,
        values: values.join(',')
      });
      const response = await fetch(`/api/sort?${query}`);
      const history = await response.json();
      if (!response.ok) {
        throw new Error(history.error || 'Falha na API de ordenação.');
      }

      animationFrames = createAnimationFromHistory(history);
    } catch (error) {
      animationFrames = [];
      element('sequenceError').textContent =
        `Não foi possível obter o histórico da API C. ${error.message}`;
      element('sequenceError').hidden = false;
      return;
    } finally {
      element('sortButton').disabled = false;
    }
    currentFrameIndex = 0;
    element('timeline').max = Math.max(0, animationFrames.length - 1);
    element('timelineEnd').textContent = `${animationFrames.length - 1} PASSOS`;
    element('emptyState').hidden = true;
    element('stepLabel').textContent =
      `${algorithmNames[Number(element('algorithm').value)]} · ${
        animationFrames.length - 1
      } passos`;
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
