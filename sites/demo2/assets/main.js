const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');
// 设置画布大小为窗口大小
canvas.width = window.innerWidth;
canvas.height = window.innerHeight;

let colorPoints = []; // 存储随机点和颜色
const regionSize = 30; // 区域大小
const pointsCount = 4; // 随机点数量
let selectedPoint = null; // 选中的点

// 随机生成颜色点
function generateColorPoints(points) {
  colorPoints = [];
  for (let i = 0; i < points; i++) {
    const x = Math.floor(Math.random() * canvas.width);
    const y = Math.floor(Math.random() * canvas.height);
    const color = [
      Math.random() * 255,
      Math.random() * 255,
      Math.random() * 255
    ];
    colorPoints.push({ x, y, color });
    // ctx.fillStyle = `rgb(${color[0]}, ${color[1]}, ${color[2]})`;
    ctx.fillStyle = `rgb(${0}, ${0}, ${0})`;
    ctx.fillRect(x, y, 20, 20); // 在该点绘制一个像素
    // console.log(x, y)
  }
}

// 使用插值算法绘制整个 Canvas
function fillCanvas() {
  for (let x = 0; x < canvas.width; x += regionSize) {
    for (let y = 0; y < canvas.height; y += regionSize) {
      let weights = [];
      let sumWeights = 0; // 用于计算权重的总和

      // 计算每个颜色点的权重
      let notvalid = false
      for (let i = 0; i < colorPoints.length; i++) {
        const colorPoint = colorPoints[i];
        if (x == colorPoint.x && y == colorPoint.y) {
          notvalid = true
        }
        const distance = Math.pow(Math.abs(x - colorPoint.x), 2) + Math.pow(Math.abs(y - colorPoint.y), 2);
        weights.push(distance);
        sumWeights += distance;
      }
      if (notvalid) {
        continue;
      }

      // 计算归一化权重，GPT写的好怪异，不管了，暂时能看，到时候自己重写一遍。
      const normalizedWeights = weights.map(value => sumWeights / value);

      // 计算插值颜色
      const color = normalizedWeights.reduce((sum, currentWeight, index) => {
        const color = colorPoints[index].color;
        return [
          sum[0] + (currentWeight * color[0]),
          sum[1] + (currentWeight * color[1]),
          sum[2] + (currentWeight * color[2])
        ];
      }, [0, 0, 0]);

      // 归一化颜色
      const totalWeight = normalizedWeights.reduce((a, b) => a + b, 0);
      // console.log(totalWeight)
      ctx.fillStyle = `rgb(${Math.round(color[0] / totalWeight)}, ${Math.round(color[1] / totalWeight)}, ${Math.round(color[2] / totalWeight)})`;
      ctx.fillRect(x, y, regionSize, regionSize); // 用插值颜色填充区域
    }
  }
  ctx.filter = `blur(${regionSize * 2}px)`; // 设置模糊值，可以根据需要调整
  ctx.drawImage(canvas, 0, 0); // 重绘整个 Canvas
  ctx.filter = 'none'; // 重置滤镜，防止影响后续绘制
}

// 处理鼠标按下或触摸事件，选中离鼠标最近的点
function onMouseDown(event) {
  const mouseX = event.clientX || event.touches[0].clientX;
  const mouseY = event.clientY || event.touches[0].clientY;
  console.log(mouseX,mouseY)

  // 找到距离鼠标最近的颜色点
  selectedPoint = colorPoints.reduce((closest, point) => {
    const distance = Math.hypot(point.x - mouseX, point.y - mouseY);
    return distance < Math.hypot(closest.x - mouseX, closest.y - mouseY) ? point : closest;
  });

  canvas.addEventListener('mousemove', onMouseMove);
  canvas.addEventListener('touchmove', onMouseMove);
}

// 处理鼠标或手指移动事件，实时更新选中点的位置
function onMouseMove(event) {
  const mouseX = event.clientX || event.touches[0].clientX;
  const mouseY = event.clientY || event.touches[0].clientY;

  if (selectedPoint) {
    // 更新选中点的坐标
    selectedPoint.x = mouseX;
    selectedPoint.y = mouseY;

    // 重新绘制整个 Canvas
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    fillCanvas();
  }
}

// 处理鼠标抬起或触摸结束事件，停止拖动
function onMouseUp() {
  selectedPoint = null;
  canvas.removeEventListener('mousemove', onMouseMove);
  canvas.removeEventListener('touchmove', onMouseMove);
}

// 初始化
function init() {
  generateColorPoints(pointsCount);
  fillCanvas();

  // 监听鼠标事件
  canvas.addEventListener('mousedown', onMouseDown);
  canvas.addEventListener('mouseup', onMouseUp);
  canvas.addEventListener('mouseleave', onMouseUp);

  // 监听触摸事件
  canvas.addEventListener('touchstart', onMouseDown);
  canvas.addEventListener('touchend', onMouseUp);
}

// 调用初始化函数
init();