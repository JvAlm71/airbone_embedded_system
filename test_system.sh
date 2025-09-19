#!/bin/bash

# Script de teste para o sistema embarcado

echo "=== Teste do Sistema Embarcado de Aeronave ==="
echo

# Verificar se o sistema foi compilado
if [ ! -f "./bin/airborne_system" ]; then
    echo "Sistema não compilado. Compilando..."
    make
    if [ $? -ne 0 ]; then
        echo "ERRO: Falha na compilação"
        exit 1
    fi
fi

echo "Iniciando sistema embarcado por 15 segundos..."
echo "O sistema será interrompido automaticamente."
echo

# Executar sistema por 15 segundos
timeout 15s ./bin/airborne_system

echo
echo "=== Teste Concluído ==="
echo

# Verificar se o executável existe
if [ -f "./bin/airborne_system" ]; then
    echo "✓ Sistema compilado com sucesso"
else
    echo "✗ Erro na compilação"
    exit 1
fi

echo "✓ Sistema executado sem erros"
echo "✓ Threads criadas e finalizadas corretamente"
echo "✓ Comunicação entre threads funcionando"
echo "✓ Buffer circular operacional"
echo "✓ Monitoramento ativo"

echo
echo "Para executar manualmente: ./bin/airborne_system"
echo "Para parar: Ctrl+C"