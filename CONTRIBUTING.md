# Guía de Contribución a SMART-GROW

¡Gracias por tu interés en colaborar con **SMART-GROW**, un proyecto de código abierto para automatización agrícola y educación técnica con ESP32-S3 e Inteligencia Artificial!

## ¿Cómo contribuir?

1. **Haz un Fork** del repositorio en GitHub.
2. **Crea una rama (branch)** para tu funcionalidad o corrección:
   ```bash
   git checkout -b feature/nueva-mejora
   ```
3. **Realiza tus cambios** manteniendo el estilo y la arquitectura del proyecto:
   - **Firmware**: Código determinista en C++ no bloqueante (`millis()`), modular por subsistemas.
   - **Dashboard**: Vanilla HTML/CSS/JS con diseño responsivo.
   - **AI Bridge**: Python con FastAPI y LM Studio client, preservando la capa de seguridad (`SafetyGuard`).
4. **Ejecuta las pruebas**:
   - Pruebas de integración y unitarias en `tests/`.
5. **Haz Commit y Push**:
   ```bash
   git commit -m "feat: descripción de la mejora"
   git push origin feature/nueva-mejora
   ```
6. **Abre un Pull Request** explicando claramente los cambios realizados.

## Reporte de Problemas
Si encuentras un bug o deseas proponer una mejora, abre un issue en GitHub detallando:
- Comportamiento observado vs. esperado.
- Versión de hardware / ESP32 utilizada.
- Pasos para reproducir.
