#!/bin/bash

# ============ test_api.sh - Validar API endpoints ============
# Use: ./test_api.sh

API_URL="http://localhost:8080/api"
TIMEOUT=5

echo "🧪 TESTE DE API - MissionLink"
echo "=============================="
echo ""

# Função para testar endpoint
test_endpoint() {
    local name=$1
    local endpoint=$2
    
    echo "📡 Testando: $name"
    echo "   URL: $API_URL$endpoint"
    
    response=$(curl -s --max-time $TIMEOUT "$API_URL$endpoint" 2>/dev/null)
    
    if [ -z "$response" ]; then
        echo "   ❌ Sem resposta (timeout ou erro)"
        return 1
    fi
    
    # Verificar se é JSON válido
    if echo "$response" | python3 -m json.tool >/dev/null 2>&1; then
        echo "   ✅ JSON válido"
        echo "   Response:"
        echo "$response" | python3 -m json.tool | head -20
        echo ""
        return 0
    else
        echo "   ❌ JSON inválido"
        echo "   Response: $response"
        echo ""
        return 1
    fi
}

# Testar todos os endpoints
echo "1️⃣  SYSTEM STATUS"
test_endpoint "System Status" "/system/status"

echo ""
echo "2️⃣  ROVERS LIST"
test_endpoint "Rovers List" "/rovers"

echo ""
echo "3️⃣  MISSIONS LIST"
test_endpoint "Missions List" "/missions"

echo ""
echo "4️⃣  TELEMETRY LATEST"
test_endpoint "Telemetry Latest" "/telemetry/latest"

echo ""
echo "=============================="
echo "✅ Teste concluído!"