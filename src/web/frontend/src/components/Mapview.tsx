import { useEffect, useRef, useState } from "react";
import * as THREE from "three";

const STRESS_POINT_COUNT = 100_000;

export default function Mapview() {
  const mountRef = useRef<HTMLDivElement>(null);

  // ---------------- UI STATE ----------------
  const [mode, setMode] = useState<"2d" | "3d">("3d");
  const [handedness, setHandedness] = useState<"lhs" | "rhs">("rhs");
  const [upAxis, setUpAxis] = useState<"y" | "z">("y");
  const [zoom, setZoom] = useState(1);
  const [pollRate, setPollRate] = useState(100);

  // ---------------- THREE REFS ----------------
  const cameraRef = useRef<THREE.OrthographicCamera | null>(null);
  const sceneRef = useRef<THREE.Scene | null>(null);
  const rendererRef = useRef<THREE.WebGLRenderer | null>(null);

  const pointsRef = useRef<THREE.Points | null>(null);
  const positionsRef = useRef<Float32Array | null>(null);

  useEffect(() => {
    const mount = mountRef.current;
    if (!mount) return;

    // ---------------- RENDERER ----------------
    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setSize(mount.clientWidth, mount.clientHeight);
    renderer.setClearColor(0x0f172a);
    mount.appendChild(renderer.domElement);
    rendererRef.current = renderer;

    // ---------------- SCENE ----------------
    const scene = new THREE.Scene();
    sceneRef.current = scene;

    // ---------------- CAMERA ----------------
    const aspect = mount.clientWidth / mount.clientHeight;
    const frustum = 500;

    const camera = new THREE.OrthographicCamera(
      -frustum * aspect,
      frustum * aspect,
      frustum,
      -frustum,
      0.1,
      5000
    );

    camera.position.set(600, 500, 600);
    camera.lookAt(0, 0, 0);

    cameraRef.current = camera;

    // ---------------- GRID ----------------
    const grid = new THREE.GridHelper(2000, 40, 0x444444, 0x222222);
    scene.add(grid);

    // ---------------- POINT CLOUD ----------------
    function createPointCloud() {
      const positions = new Float32Array(STRESS_POINT_COUNT * 3);

      const geometry = new THREE.BufferGeometry();
      geometry.setAttribute(
        "position",
        new THREE.BufferAttribute(positions, 3)
      );

      const material = new THREE.PointsMaterial({
        color: 0x38bdf8,
        size: 2,
        sizeAttenuation: false,
      });

      const points = new THREE.Points(geometry, material);
      scene.add(points);

      pointsRef.current = points;
      positionsRef.current = positions;
    }

    createPointCloud();

    // ---------------- FETCH ----------------
    async function fetchEntityPositions() {
      try {
        console.log("fetchEntityPositions");

        const res = await fetch("/api/entity-fetch");
        const data = await res.json();
        console.log("Received entity positions", data);

        const positions = positionsRef.current;
        if (!positions) return;

        const points = data.points;
        const len = Math.min(points.length, STRESS_POINT_COUNT);

        for (let i = 0; i < len; i++) {
          const dst = i * 3;
          const p = points[i];

          positions[dst] = p[0];
          positions[dst + 1] = 2;
          positions[dst + 2] = p[1];
        }

        const geometry = pointsRef.current?.geometry;
        if (geometry) {
          geometry.attributes.position.needsUpdate = true;
        }
      } catch (e) {
        console.error("fetchEntityPositions failed", e);
      }
    }

    // ---------------- POLLING (FIXED) ----------------
    let intervalId: ReturnType<typeof setInterval> | null = null;

    function startPolling(rate: number) {
      if (intervalId) clearInterval(intervalId);

      intervalId = setInterval(() => {
        fetchEntityPositions();
      }, rate);
    }

    startPolling(pollRate);

    // ---------------- CONTROLS ----------------
    let dragging = false;
    let lastX = 0;
    let lastY = 0;

    function onMouseDown(e: MouseEvent) {
      dragging = true;
      lastX = e.clientX;
      lastY = e.clientY;
    }

    function onMouseUp() {
      dragging = false;
    }

    function onMouseMove(e: MouseEvent) {
      if (!dragging || !cameraRef.current) return;

      const camera = cameraRef.current;

      const dx = e.clientX - lastX;
      const dy = e.clientY - lastY;

      lastX = e.clientX;
      lastY = e.clientY;

      const scale = (camera.right - camera.left) / mount.clientWidth;

      const right = new THREE.Vector3();
      const up = new THREE.Vector3();

      camera.matrixWorld.extractBasis(right, up, new THREE.Vector3());

      right.multiplyScalar(-dx * scale);
      up.multiplyScalar(dy * scale);

      camera.position.add(right);
      camera.position.add(up);
    }

    function onWheel(e: WheelEvent) {
      if (!cameraRef.current) return;
      e.preventDefault();

      cameraRef.current.zoom *= e.deltaY > 0 ? 0.9 : 1.1;
      cameraRef.current.zoom = Math.max(0.2, Math.min(cameraRef.current.zoom, 10));
      cameraRef.current.updateProjectionMatrix();
    }

    renderer.domElement.addEventListener("mousedown", onMouseDown);
    window.addEventListener("mouseup", onMouseUp);
    window.addEventListener("mousemove", onMouseMove);
    renderer.domElement.addEventListener("wheel", onWheel, { passive: false });

    // ---------------- RESIZE ----------------
 

    function onResize() {
      if (!cameraRef.current || !rendererRef.current) return;

      const aspect = mount.clientWidth / mount.clientHeight;

      cameraRef.current.left = -frustum * aspect;
      cameraRef.current.right = frustum * aspect;
      cameraRef.current.top = frustum;
      cameraRef.current.bottom = -frustum;

      cameraRef.current.updateProjectionMatrix();
      rendererRef.current.setSize(mount.clientWidth, mount.clientHeight);
    }

    window.addEventListener("resize", onResize);

    // ---------------- LOOP ----------------
    let frame = 0;

    function animate() {
      frame = requestAnimationFrame(animate);
      renderer.render(scene, camera);
    }

    animate();

    // ---------------- CLEANUP ----------------
    return () => {
      cancelAnimationFrame(frame);

      window.removeEventListener("resize", onResize);
      window.removeEventListener("mouseup", onMouseUp);
      window.removeEventListener("mousemove", onMouseMove);

      if (intervalId) clearInterval(intervalId);

      renderer.dispose();
      mount.removeChild(renderer.domElement);
    };
  }, [pollRate]); // IMPORTANT FIX

  // ---------------- UI EFFECTS ----------------
  useEffect(() => {
    const camera = cameraRef.current;
    if (!camera) return;

    if (mode === "2d") {
      camera.position.set(0, 800, 0);
      camera.lookAt(0, 0, 0);
      camera.zoom = zoom;
    } else {
      camera.position.set(600, 500, 600);
      camera.lookAt(0, 0, 0);
    }

    camera.updateProjectionMatrix();
  }, [mode, zoom]);

  useEffect(() => {
    const scene = sceneRef.current;
    if (!scene) return;
    scene.scale.x = handedness === "lhs" ? -1 : 1;
  }, [handedness]);

  useEffect(() => {
    const scene = sceneRef.current;
    if (!scene) return;
    scene.rotation.x = upAxis === "z" ? -Math.PI / 2 : 0;
  }, [upAxis]);

  return (
    <div className="card">
      <div className="card-header">
        <h3 className="card-title">AtlasNet 3D Map</h3>
      </div>

      <div className="card-body p-2">
        {/* CONTROLS */}
        <div className="d-flex gap-2 mb-2 flex-wrap">

          <div className="btn-group">
            <button className={`btn btn-sm ${mode === "2d" ? "btn-primary" : "btn-outline-primary"}`} onClick={() => setMode("2d")}>2D</button>
            <button className={`btn btn-sm ${mode === "3d" ? "btn-primary" : "btn-outline-primary"}`} onClick={() => setMode("3d")}>3D</button>
          </div>

          <div className="btn-group">
            <button className={`btn btn-sm ${handedness === "lhs" ? "btn-primary" : "btn-outline-primary"}`} onClick={() => setHandedness("lhs")}>LHS</button>
            <button className={`btn btn-sm ${handedness === "rhs" ? "btn-primary" : "btn-outline-primary"}`} onClick={() => setHandedness("rhs")}>RHS</button>
          </div>

          <div className="btn-group">
            <button className={`btn btn-sm ${upAxis === "y" ? "btn-primary" : "btn-outline-primary"}`} onClick={() => setUpAxis("y")}>Y-Up</button>
            <button className={`btn btn-sm ${upAxis === "z" ? "btn-primary" : "btn-outline-primary"}`} onClick={() => setUpAxis("z")}>Z-Up</button>
          </div>

          <div className="d-flex align-items-center gap-2">
            <span className="small text-muted">Poll {pollRate}ms</span>
            <input
              type="range"
              className="form-range"
              min={1}
              max={1000}
              step={1}
              value={pollRate}
              onChange={(e) => setPollRate(Number(e.target.value))}
              style={{ width: 200 }}
            />
          </div>

          {mode === "2d" && (
            <div className="d-flex align-items-center gap-2">
              <span className="small text-muted">Zoom</span>
              <input
                type="range"
                className="form-range"
                min={0.2}
                max={5}
                step={0.1}
                value={zoom}
                onChange={(e) => setZoom(Number(e.target.value))}
                style={{ width: 150 }}
              />
            </div>
          )}
        </div>

        {/* MAP */}
        <div
          ref={mountRef}
          style={{ width: "100%", height: "700px", cursor: "grab" }}
        />
      </div>
    </div>
  );
}