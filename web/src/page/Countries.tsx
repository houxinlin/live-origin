import { useEffect, useRef, useState } from "react";
import Globe from "../gl";
import * as THREE from 'three';
import { log } from "three/tsl";

interface CountryProperties {
    NAME: string;
    ISO_A2: string;
    ISO_A3: string;
    ADMIN: string;
}

interface CountryFeature {
    type: "Feature";
    properties: CountryProperties;
    geometry: any;
}

interface LocationData {
    from: {
        lat: number;
        lng: number;
        city?: string;
        country?: string;
    };
    to: {
        lat: number;
        lng: number;
        city?: string;
        country?: string;
    };
}

interface CityInfo {
    city: string;
    country: string;
    timestamp: number;
    type: 'from' | 'to';
}

export default function Countries() {
    const globeRef = useRef<any>(null);
    const wsRef = useRef<WebSocket | null>(null);
    const [hoveredCountry, setHoveredCountry] = useState<string | null>(null);
    const [countries, setCountries] = useState<CountryFeature[]>([]);
    const [chinaProvinces, setChinaProvinces] = useState<CountryFeature[]>([]);
    const [wsConnected, setWsConnected] = useState<boolean>(false);
    const [fps, setFps] = useState<number>(0);
    const [cityList, setCityList] = useState<CityInfo[]>([]);

    useEffect(() => {
        console.log("执行");
        // 飞线动画参数
        const ARC_REL_LEN = 0.4; // relative to whole arc
        const FLIGHT_TIME = 1000;
        const NUM_RINGS = 3;
        const RINGS_MAX_R = 5; // deg
        const RING_PROPAGATION_SPEED = 5; // deg/sec

        // 加载GeoJSON数据
        const loadCountriesData = async () => {
            try {
                const response = await fetch('/src/assets/countries.geojson');
                const data = await response.json();
                setCountries(data.features);
                return data.features;
            } catch (error) {
                console.error('加载国家数据失败:', error);
                return [];
            }
        };
        const initGlobe = async () => {
            const countriesData = await loadCountriesData();
            const material = new THREE.MeshLambertMaterial({
                color: '#000814',   // 注意：这里是红色，不是注释里的深海蓝
                opacity: 0.8,
                transparent: true,  // ⚠️ 要想生效半透明，必须加这个
                fog: false,
                flatShading: true
              });
            const globe = new Globe(document.getElementById('countriesGlobe') as HTMLElement)
                .showGlobe(true)
                .globeMaterial(material)
                .polygonsData(countriesData)
                // 优化多边形渲染性能
                .polygonsTransitionDuration(0) // 禁用过渡动画
                .polygonCapColor((d: any) => 
                    hoveredCountry === d.properties?.NAME ? 'rgba(255, 140, 0, 0.9)' : 'rgba(100, 150, 200, 0.6)'
                )
                .polygonSideColor(() => 'rgb(7, 39, 71)')
                .polygonStrokeColor(() => '#ffffff')
                .polygonAltitude(() => 0.01) // 稍微抬高多边形
                // 添加飞线相关配置
                .arcColor(() => '#00ff00')
                .ringColor(() => (t: number) => `rgba(0,255,0,${1 - t})`)
                .ringAltitude(0.02) // 设置环形高度，让它们显示在地球表面之上
                .polygonLabel((d: any) => `
                    <div style="
                        background: rgba(0, 0, 0, 0.8);
                        color: white;
                        padding: 8px 12px;
                        border-radius: 4px;
                        font-size: 14px;
                        font-family: Arial, sans-serif;
                        box-shadow: 0 2px 10px rgba(0,0,0,0.5);
                    ">
                        <b>${d.properties?.NAME || d.properties?.ADMIN}</b><br/>
                        代码: ${d.properties?.ISO_A3 || 'N/A'}
                    </div>
                `)
                .arcDashLength(ARC_REL_LEN)
                .arcDashGap(2)
                .arcDashInitialGap(1)
                .arcDashAnimateTime(FLIGHT_TIME)
                .arcsTransitionDuration(0)
                .ringMaxRadius(RINGS_MAX_R)
                .ringPropagationSpeed(RING_PROPAGATION_SPEED)
                .ringRepeatPeriod(FLIGHT_TIME * ARC_REL_LEN / NUM_RINGS)
                .width(window.innerWidth)
                .height(window.innerHeight);

            globeRef.current = globe;

            // 添加中国省份边界
            // if (provincesData.length > 0) {
            //     // 创建第二个多边形层用于显示中国省份
            //     globe.polygonsData([...countriesData, ...provincesData])
            //         .polygonCapColor((d: any) => {
            //             // 中国省份使用不同的颜色
            //             if (d.properties?.adcode) {
            //                 return hoveredCountry === d.properties?.name ? 'rgba(255, 215, 0, 0.8)' : 'rgba(255, 255, 255, 0.1)';
            //             }
            //             // 国家使用原来的颜色
            //             return hoveredCountry === d.properties?.NAME ? 'rgba(255, 140, 0, 0.9)' : 'rgba(100, 150, 200, 0.6)';
            //         })
            //         .polygonSideColor((d: any) => {
            //             if (d.properties?.adcode) {
            //                 return 'rgba(255, 255, 255, 0.2)';
            //             }
            //             return 'rgb(7, 39, 71)';
            //         })
            //         .polygonStrokeColor((d: any) => {
            //             if (d.properties?.adcode) {
            //                 return '#ffff00'; // 中国省份边框为黄色
            //             }
            //             return '#ffffff'; // 国家边框为白色
            //         })
            //         .polygonAltitude((d: any) => {
            //             if (d.properties?.adcode) {
            //                 return 0.02; // 中国省份稍微高一点
            //             }
            //             return 0.01; // 国家边界
            //         });
            // }
            // 添加环境光照
            const scene = globe.scene();
            globe.pointOfView({ lat: 23.2955, lng: 113.825, altitude: 2 });

            const controls = globe.controls();
            // controls.autoRotate = true;
            // controls.autoRotateSpeed = 0.3; // 降低旋转速度
            // controls.enableDamping = true; // 启用阻尼
            // controls.dampingFactor = 0.1; // 设置阻尼系数
            
            // 限制缩放和旋转范围以提高性能
            controls.minDistance = 200;
            controls.maxDistance = 800;
            controls.maxPolarAngle = Math.PI; // 允许完全旋转


        };

        function drawArcFromData(data: LocationData) {
            const { from, to } = data;
            const startLat = from.lat;
            const startLng = from.lng;
            const endLat = to.lat;
            const endLng = to.lng;
            let globe = globeRef.current;
            console.log(globeRef.current);
            
            // 添加城市信息到列表
            const timestamp = Date.now();
            const newCities: CityInfo[] = [];
            
            if (from.city && from.country) {
                newCities.push({
                    city: from.city,
                    country: from.country,
                    timestamp,
                    type: 'from'
                });
            }
            if (newCities.length > 0) {
                setCityList(prev => {
                    const updated = [...newCities, ...prev];
                    console.log(updated);
                    
                    // 只保留最新的20条记录
                    return updated.slice(0, 20);
                });
            }
            
            // 添加弧线
            const arc = { startLat, startLng, endLat, endLng };
            globe.arcsData([...globe.arcsData(), arc]);
            setTimeout(() => globe.arcsData(globe.arcsData().filter((d: any) => d !== arc)), FLIGHT_TIME * 2);

            // 添加起点光环
            const srcRing = { lat: startLat, lng: startLng };
            globe.ringsData([...globe.ringsData(), srcRing]);
            setTimeout(() => globe.ringsData(globe.ringsData().filter((r: any) => r !== srcRing)), FLIGHT_TIME * ARC_REL_LEN);

            // 添加终点光环
            setTimeout(() => {
                const targetRing = { lat: endLat, lng: endLng };
                globe.ringsData([...globe.ringsData(), targetRing]);
                setTimeout(() => globe.ringsData(globe.ringsData().filter((r: any) => r !== targetRing)), FLIGHT_TIME * ARC_REL_LEN);
            }, FLIGHT_TIME);
        }
        // WebSocket连接
        const connectWebSocket = () => {
            // 如果已经有连接，先关闭
            if (wsRef.current && wsRef.current.readyState !== WebSocket.CLOSED) {
                wsRef.current.close();
            }
            
            try {
                const ws = new WebSocket(`ws://${location.host}`); 
                wsRef.current = ws;

                ws.onopen = () => {
                    console.log('Countries页面: WebSocket连接已建立');
                    setWsConnected(true);
                };

                ws.onmessage = (event) => {
                    try {
                        const data: LocationData = JSON.parse(event.data);
                        console.log('Countries页面收到数据:', data);
                        
                        
                        if (data.from && data.to && 
                            typeof data.from.lat === 'number' && typeof data.from.lng === 'number' &&
                            typeof data.to.lat === 'number' && typeof data.to.lng === 'number') {
                            drawArcFromData(data);
                        } else {
                            console.warn('数据格式不正确:', data);
                        }
                    } catch (error) {
                        console.error('解析WebSocket数据失败:', error);
                    }
                };

                ws.onclose = (event) => {
                    console.log('Countries页面: WebSocket连接已关闭:', event.code, event.reason);
                    setWsConnected(false);

                };

                ws.onerror = (error) => {
                    console.error('Countries页面: WebSocket错误:', error);
                };
            } catch (error) {
                console.error('Countries页面: 创建WebSocket连接失败:', error);

            }
        };

        initGlobe();

        // 建立WebSocket连接
        connectWebSocket();

        // 窗口大小改变时调整Globe大小
        const handleResize = () => {
            if (globeRef.current) {
                globeRef.current
                    .width(window.innerWidth)
                    .height(window.innerHeight);
            }
        };

        window.addEventListener('resize', handleResize);

        // 清理函数
        return () => {
            console.log("销毁",wsRef.current);
            
            window.removeEventListener('resize', handleResize);
            document.body.style.cursor = 'default';
            // 确保WebSocket连接被正确关闭
            if (wsRef.current && wsRef.current.readyState !== WebSocket.CLOSED) {
                wsRef.current.close();
                wsRef.current = null;
            }
        };
    }, []);


    return (
        <div style={{ 
            position: 'relative', 
            width: '100vw', 
            height: '100vh', 
            overflow: 'hidden',
            backgroundColor: '#0a0a0a' // 深色背景
        }}>
            <style>{`
                @keyframes scaleIn {
                    from { 
                        opacity: 0; 
                        transform: scale(0.3) translateX(-20px); 
                    }
                    to { 
                        opacity: 1; 
                        transform: scale(1) translateX(0); 
                    }
                }
                
                /* 自定义滚动条样式 */
                div::-webkit-scrollbar {
                    width: 6px;
                }
                div::-webkit-scrollbar-track {
                    background: rgba(255,255,255,0.1);
                    border-radius: 3px;
                }
                div::-webkit-scrollbar-thumb {
                    background: rgba(255,255,255,0.3);
                    border-radius: 3px;
                }
                div::-webkit-scrollbar-thumb:hover {
                    background: rgba(255,255,255,0.5);
                }
            `}</style>
            <div id="countriesGlobe" style={{ width: '100%', height: '100%' }}></div>
            
            {/* 左侧城市列表 */}
            {/* <div style={{
                position: 'absolute',
                left: '20px',
                top: '20px',
                bottom: '20px',
                width: '300px',
                background: 'rgba(0, 0, 0, 0.8)',
                color: 'white',
                borderRadius: '8px',
                padding: '15px',
                fontFamily: 'Arial, sans-serif',
                fontSize: '14px',
                zIndex: 1000,
                display: 'flex',
                flexDirection: 'column'
            }}>
                <h3 style={{ margin: '0 0 15px 0', fontSize: '18px', borderBottom: '1px solid rgba(255,255,255,0.3)', paddingBottom: '10px' }}>
                    📍 实时起点城市
                </h3>
                
                <div style={{ 
                    flex: 1, 
                    overflowY: 'auto',
                    scrollbarWidth: 'thin',
                    scrollbarColor: 'rgba(255,255,255,0.3) transparent'
                }}>
                    {cityList.length === 0 ? (
                        <div style={{ 
                            textAlign: 'center', 
                            color: 'rgba(255,255,255,0.6)',
                            marginTop: '50px'
                        }}>
                            等待数据连接...
                        </div>
                    ) : (
                        cityList.map((cityInfo, index) => (
                            <div
                                key={`${cityInfo.timestamp}-${index}`}
                                style={{
                                    padding: '8px 12px',
                                    marginBottom: '8px',
                                    borderRadius: '6px',
                                    background: 'rgba(0, 255, 0, 0.1)',
                                    border: '1px solid rgba(0, 255, 0, 0.3)',
                                    animation: index === 0 ? 'scaleIn 0.6s cubic-bezier(0.175, 0.885, 0.32, 1.275)' : 'none'
                                }}
                            >
                                <div style={{ 
                                    display: 'flex', 
                                    alignItems: 'center',
                                    justifyContent: 'space-between'
                                }}>
                                    <div>
                                        <div style={{ 
                                            fontWeight: 'bold',
                                            color: '#00ff00'
                                        }}>
                                            📍 起点城市
                                        </div>
                                        <div style={{ fontSize: '16px', margin: '4px 0' }}>
                                            {cityInfo.city}
                                        </div>
                                        <div style={{ 
                                            fontSize: '12px', 
                                            color: 'rgba(255,255,255,0.7)'
                                        }}>
                                            {cityInfo.country}
                                        </div>
                                    </div>
                                    <div style={{ 
                                        fontSize: '10px', 
                                        color: 'rgba(255,255,255,0.5)',
                                        textAlign: 'right'
                                    }}>
                                        {new Date(cityInfo.timestamp).toLocaleTimeString()}
                                    </div>
                                </div>
                            </div>
                        ))
                    )}
                </div>
                
                <div style={{
                    marginTop: '10px',
                    padding: '8px',
                    borderTop: '1px solid rgba(255,255,255,0.3)',
                    fontSize: '12px',
                    color: 'rgba(255,255,255,0.7)',
                    textAlign: 'center'
                }}>
                    显示最新 {cityList.length}/20 条记录
                </div>
            </div> */}
            
            {/* 信息面板 */}
            <div style={{
                position: 'absolute',
                top: '20px',
                right: '20px',
                background: 'rgba(0, 0, 0, 0.7)',
                color: 'white',
                padding: '15px',
                borderRadius: '8px',
                fontFamily: 'Arial, sans-serif',
                fontSize: '14px',
                maxWidth: '300px',
                zIndex: 1000
            }}>
                <div style={{
                    marginTop: '8px',
                    padding: '4px 8px',
                    borderRadius: '4px',
                    fontSize: '12px',
                    backgroundColor: wsConnected ? 'rgba(0, 255, 0, 0.2)' : 'rgba(255, 0, 0, 0.2)',
                    border: `1px solid ${wsConnected ? 'rgba(0, 255, 0, 0.5)' : 'rgba(255, 0, 0, 0.5)'}`
                }}>
                    WebSocket: {wsConnected ? '🟢 已连接' : '🔴 未连接'}
                </div>
                {hoveredCountry && (
                    <div style={{
                        marginTop: '10px',
                        padding: '8px',
                        background: 'rgba(255, 100, 0, 0.2)',
                        borderRadius: '4px',
                        border: '1px solid rgba(255, 100, 0, 0.5)'
                    }}>
                        <strong>当前悬停: {hoveredCountry}</strong>
                    </div>
                )}
            </div>

            {/* 控制面板 */}
            <div style={{
                position: 'absolute',
                bottom: '20px',
                right: '20px',
                background: 'rgba(0, 0, 0, 0.7)',
                color: 'white',
                padding: '10px',
                borderRadius: '8px',
                fontFamily: 'Arial, sans-serif',
                fontSize: '12px',
                zIndex: 1000
            }}>
                <div>已加载 {countries.length} 个国家/地区</div>
                <div style={{
                    marginTop: '5px',
                    padding: '2px 6px',
                    borderRadius: '3px',
                    backgroundColor: fps >= 30 ? 'rgba(0, 255, 0, 0.3)' : fps >= 20 ? 'rgba(255, 255, 0, 0.3)' : 'rgba(255, 0, 0, 0.3)'
                }}>
                    FPS: {fps}
                </div>
            </div>
        </div>
    );
}
