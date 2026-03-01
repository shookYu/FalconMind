const { createApp, ref, reactive, computed, onMounted } = Vue;

createApp({
  setup() {
    // 响应式数据
    const templates = ref([]);
    const nodes = reactive([]);
    const edges = reactive([]);
    const selectedNodeId = ref(null);
    const draggingNodeId = ref(null);
    const connectingFrom = ref(null);
    const currentProject = ref(null);
    const currentFlow = ref(null);
    
    // 项目/流程管理
    const projects = ref([]);
    const flows = ref([]);
    const selectedProjectId = ref(null);
    const selectedFlowId = ref(null);
    const isLoading = ref(false);
    
    // 节点搜索和过滤
    const searchKeyword = ref('');
    const selectedCategory = ref(''); // 空字符串表示"全部"
    const nodeCategories = [
      { value: '', label: '全部类别' },
      { value: 'FLIGHT', label: '飞行控制' },
      { value: 'SENSORS', label: '传感器' },
      { value: 'PERCEPTION', label: '感知' },
      { value: 'MISSION', label: '任务规划' },
      { value: 'LOGIC', label: '逻辑' },
      { value: 'UTILITY', label: '工具' },
    ];
    
    // 保存状态提示
    const saveStatus = ref('saved'); // 'saved' | 'saving' | 'unsaved' | 'error'
    let autoSaveTimer = null;
    const autoSaveInterval = window.BuilderConfig?.autoSaveInterval || 2000;
    
    // Toast通知系统
    const notifications = ref([]);
    const notificationDuration = window.BuilderConfig?.notificationDuration || 3000;
    
    // 显示Toast通知
    function showToast(message, type = 'info') {
      const id = Date.now();
      const notification = {
        id,
        message,
        type, // 'info' | 'success' | 'warning' | 'error'
        timestamp: Date.now()
      };
      notifications.value.push(notification);
      
      // 自动移除
      setTimeout(() => {
        const index = notifications.value.findIndex(n => n.id === id);
        if (index !== -1) {
          notifications.value.splice(index, 1);
        }
      }, notificationDuration);
      
      return id;
    }
    
    // 移除通知
    function removeNotification(id) {
      const index = notifications.value.findIndex(n => n.id === id);
      if (index !== -1) {
        notifications.value.splice(index, 1);
      }
    }
    
    // 撤销/重做功能
    const history = ref([]); // 历史记录栈
    const historyIndex = ref(-1); // 当前历史记录索引
    const maxHistorySize = window.BuilderConfig?.maxHistorySize || 50;
    
    // 画布变换
    const canvasTransform = reactive({
      x: 0,
      y: 0,
      scale: 1,
    });
    const isPanning = ref(false);
    const panStart = ref({ x: 0, y: 0 });

    // 获取选中的节点
    const selectedNode = computed(() => {
      return nodes.find(n => n.node_id === selectedNodeId.value);
    });
    
    // 过滤后的模板列表
    const filteredTemplates = computed(() => {
      let result = templates.value;
      
      // 按类别过滤
      if (selectedCategory.value) {
        result = result.filter(t => t.category === selectedCategory.value);
      }
      
      // 按关键词搜索（搜索名称、描述、template_id）
      if (searchKeyword.value.trim()) {
        const keyword = searchKeyword.value.trim().toLowerCase();
        result = result.filter(t => {
          const name = (t.name || '').toLowerCase();
          const description = (t.description || '').toLowerCase();
          const templateId = (t.template_id || '').toLowerCase();
          return name.includes(keyword) || description.includes(keyword) || templateId.includes(keyword);
        });
      }
      
      return result;
    });

    // 更新节点参数
    function updateNodeParameters(newParams) {
      if (selectedNode.value) {
        if (!selectedNode.value.parameters) {
          selectedNode.value.parameters = {};
        }
        Object.assign(selectedNode.value.parameters, newParams);
      }
    }

    // 获取嵌套对象的值
    function getNestedValue(obj, path) {
      const keys = path.split('.');
      let current = obj || {};
      for (const key of keys) {
        if (current[key] === undefined) {
          return undefined;
        }
        current = current[key];
      }
      return current;
    }

    // 设置嵌套对象的值
    function setNestedValue(obj, path, value) {
      const keys = path.split('.');
      const newObj = JSON.parse(JSON.stringify(obj || {}));
      let current = newObj;
      
      for (let i = 0; i < keys.length - 1; i++) {
        if (!current[keys[i]]) {
          current[keys[i]] = {};
        }
        current = current[keys[i]];
      }
      
      current[keys[keys.length - 1]] = value;
      return newObj;
    }

    // 获取API基础URL（从配置）
    const getApiUrl = (path) => {
      const baseUrl = window.BuilderConfig?.apiBaseUrl || 'http://127.0.0.1:9001';
      return `${baseUrl}${path.startsWith('/') ? path : '/' + path}`;
    };

    // 初始化：加载节点模板
    async function loadTemplates() {
      try {
        const response = await fetch(getApiUrl("/templates"));
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        templates.value = data.templates;
        console.log("Templates loaded:", templates.value.length);
      } catch (e) {
        console.error("Failed to load templates", e);
        alert("Failed to load templates. Please check if backend is running at " + getApiUrl(""));
      }
    }

    // 加载项目列表
    async function loadProjects() {
      try {
        isLoading.value = true;
        const response = await fetch(getApiUrl("/projects"));
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        projects.value = data.projects || [];
        console.log("Projects loaded:", projects.value.length);
      } catch (e) {
        console.error("Failed to load projects", e);
        alert("Failed to load projects: " + e.message);
      } finally {
        isLoading.value = false;
      }
    }

    // 加载流程列表
    async function loadFlows(projectId) {
      if (!projectId) {
        flows.value = [];
        return;
      }
      try {
        isLoading.value = true;
        const response = await fetch(getApiUrl(`/projects/${projectId}/flows`));
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        // 后端返回的是简化版流程列表，需要确保有version字段
        flows.value = (data.flows || []).map(f => ({
          flow_id: f.flow_id,
          name: f.name,
          description: f.description || '',
          version: f.version || '1.0', // 确保有version字段
          created_at: f.created_at,
          updated_at: f.updated_at
        }));
        console.log("Flows loaded:", flows.value.length);
      } catch (e) {
        console.error("Failed to load flows", e);
        alert("Failed to load flows: " + e.message);
      } finally {
        isLoading.value = false;
      }
    }

    // 加载项目
    async function loadProject(projectId) {
      if (!projectId) {
        currentProject.value = null;
        currentFlow.value = null;
        nodes.splice(0, nodes.length);
        edges.splice(0, edges.length);
        return;
      }
      try {
        isLoading.value = true;
        const response = await fetch(getApiUrl(`/projects/${projectId}`));
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        currentProject.value = await response.json();
        selectedProjectId.value = projectId;
        // 加载该项目的流程列表
        await loadFlows(projectId);
        console.log("Project loaded:", currentProject.value);
      } catch (e) {
        console.error("Failed to load project", e);
        alert("Failed to load project: " + e.message);
      } finally {
        isLoading.value = false;
      }
    }

    // 加载流程
    async function loadFlow(flowId) {
      if (!flowId || !currentProject.value) {
        return;
      }
      try {
        isLoading.value = true;
        const response = await fetch(getApiUrl(`/projects/${currentProject.value.project_id}/flows/${flowId}`));
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        // 后端可能直接返回flow对象，也可能返回{flow: ...}格式
        const flow = data.flow || data;
        if (!flow) {
          throw new Error("Flow data is empty");
        }
        currentFlow.value = flow;
        selectedFlowId.value = flowId;
        // 加载节点和连接
        // 注意：nodes和edges是reactive数组，需要清空后重新填充
        nodes.splice(0, nodes.length);
        edges.splice(0, edges.length);
        if (flow.nodes && Array.isArray(flow.nodes)) {
          console.log("Loading nodes:", flow.nodes.length, flow.nodes);
          // 确保每个节点都有position属性
          flow.nodes.forEach(node => {
            if (!node.position) {
              node.position = { x: 100, y: 100 };
            }
            nodes.push(node);
          });
        }
        if (flow.edges && Array.isArray(flow.edges)) {
          console.log("Loading edges:", flow.edges.length, flow.edges);
          edges.push(...flow.edges);
        }
        console.log("Flow loaded:", currentFlow.value);
        // 重置保存状态
        saveStatus.value = 'saved';
        console.log("Nodes in canvas:", nodes.length);
        console.log("Edges in canvas:", edges.length);
      } catch (e) {
        console.error("Failed to load flow", e);
        showToast("加载流程失败: " + e.message, 'error');
      } finally {
        isLoading.value = false;
      }
    }

    // 创建新工程
    async function createProject() {
      try {
        isLoading.value = true;
        const response = await fetch(getApiUrl("/projects"), {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            name: `Project ${new Date().toLocaleString()}`,
            description: "New project",
          }),
        });
        
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        currentProject.value = data.project;
        selectedProjectId.value = data.project.project_id;
        console.log("Project created:", currentProject.value);
        
        // 重新加载项目列表
        await loadProjects();
        
        // 创建默认流程
        await createFlow();
      } catch (e) {
        console.error("Failed to create project", e);
        showToast("创建项目失败: " + e.message, 'error');
      } finally {
        isLoading.value = false;
      }
    }

    // 创建新流程
    async function createFlow() {
      if (!currentProject.value) {
        console.error("Cannot create flow: no project");
        return;
      }
      
      try {
        isLoading.value = true;
        const response = await fetch(getApiUrl(`/projects/${currentProject.value.project_id}/flows`), {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            name: "Main Flow",
            description: "Main pipeline flow",
            nodes: [],
            edges: [],
          }),
        });
        
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        // 后端可能直接返回flow对象，也可能返回{flow: ...}格式
        const flow = data.flow || data;
        currentFlow.value = flow;
        selectedFlowId.value = flow.flow_id;
        // 清空并重新加载节点和连接
        nodes.splice(0, nodes.length);
        edges.splice(0, edges.length);
        if (flow.nodes && Array.isArray(flow.nodes)) {
          flow.nodes.forEach(node => {
            if (!node.position) {
              node.position = { x: 100, y: 100 };
            }
            nodes.push(node);
          });
        }
        if (flow.edges && Array.isArray(flow.edges)) {
          edges.push(...flow.edges);
        }
        
        // 重新加载流程列表
        await loadFlows(currentProject.value.project_id);
        // 重置保存状态
        saveStatus.value = 'saved';
        
        console.log("Flow created:", currentFlow.value);
      } catch (e) {
        console.error("Failed to create flow", e);
        showToast("创建流程失败: " + e.message, 'error');
      } finally {
        isLoading.value = false;
      }
    }

    // 从模板创建节点
    function createNodeFromTemplate(template, x, y) {
      const nodeId = `node_${Date.now()}`;
      const node = {
        node_id: nodeId,
        template_id: template.template_id,
        position: { x, y },
        parameters: {},
      };
      nodes.push(node);
      saveState(); // 保存状态
      return node;
    }

    // 拖拽开始（从模板库）
    function onTemplateDragStart(event, template) {
      event.dataTransfer.setData("template", JSON.stringify(template));
    }

    // 拖拽到画布
    function onCanvasDrop(event) {
      event.preventDefault();
      const rect = event.currentTarget.getBoundingClientRect();
      // 考虑画布变换
      const x = (event.clientX - rect.left - canvasTransform.x) / canvasTransform.scale;
      const y = (event.clientY - rect.top - canvasTransform.y) / canvasTransform.scale;
      
      const templateData = event.dataTransfer.getData("template");
      if (templateData) {
        const template = JSON.parse(templateData);
        createNodeFromTemplate(template, x - 75, y - 50);
      }
    }

    function onCanvasDragOver(event) {
      event.preventDefault();
    }

    // 节点拖拽
    function onNodeMouseDown(event, node) {
      if (event.button !== 0) return; // 只处理左键
      // 如果点击的是删除按钮，不处理
      if (event.target.classList.contains('btn-delete-node') || event.target.closest('.btn-delete-node')) {
        return;
      }
      event.stopPropagation(); // 阻止触发画布平移
      
      selectedNodeId.value = node.node_id;
      selectedEdgeId.value = null; // 取消连接线选择
      draggingNodeId.value = node.node_id;
      
      const rect = event.currentTarget.getBoundingClientRect();
      const startX = event.clientX - (node.position.x * canvasTransform.scale + canvasTransform.x);
      const startY = event.clientY - (node.position.y * canvasTransform.scale + canvasTransform.y);
      
      let hasMoved = false;
      function onMouseMove(e) {
        if (draggingNodeId.value === node.node_id) {
          const newX = (e.clientX - canvasTransform.x - startX) / canvasTransform.scale;
          const newY = (e.clientY - canvasTransform.y - startY) / canvasTransform.scale;
          node.position.x = Math.max(0, newX);
          node.position.y = Math.max(0, newY);
          hasMoved = true;
        }
      }
      
      function onMouseUp() {
        if (hasMoved) {
          saveState(); // 节点移动结束后保存状态
          scheduleAutoSave(); // 触发自动保存
        }
        draggingNodeId.value = null;
        document.removeEventListener("mousemove", onMouseMove);
        document.removeEventListener("mouseup", onMouseUp);
      }
      
      document.addEventListener("mousemove", onMouseMove);
      document.addEventListener("mouseup", onMouseUp);
    }

    // 获取端口类型
    function getPortType(node, portName, isOutput) {
      const template = getNodeTemplate(node.template_id);
      if (!template) return null;
      const ports = isOutput ? template.output_ports : template.input_ports;
      const port = ports.find(p => p.name === portName);
      return port ? port.type : null;
    }

    // 检查是否可以连接
    function canConnect(fromNode, fromPort, toNode, toPort) {
      const fromType = getPortType(fromNode, fromPort, true);
      const toType = getPortType(toNode, toPort, false);
      
      if (!fromType || !toType) {
        return false;
      }
      
      // 类型必须匹配或toPort接受ANY类型
      if (toType === 'ANY') return true;
      if (fromType === toType) return true;
      
      return false;
    }

    // 端口连接
    function onPortMouseDown(event, node, port, isOutput) {
      event.stopPropagation();
      if (isOutput) {
        connectingFrom.value = { node_id: node.node_id, port: port.name };
      }
    }

    function onPortMouseUp(event, node, port, isOutput) {
      event.stopPropagation();
      if (!isOutput && connectingFrom.value) {
        // 获取源节点和目标节点
        const fromNode = nodes.find(n => n.node_id === connectingFrom.value.node_id);
        const toNode = node;
        
        if (!fromNode) {
          connectingFrom.value = null;
          return;
        }
        
        // 验证连接类型
        if (!canConnect(fromNode, connectingFrom.value.port, toNode, port.name)) {
          const fromType = getPortType(fromNode, connectingFrom.value.port, true);
          const toType = getPortType(toNode, port.name, false);
          showToast(`无法连接: 端口类型不兼容 (${fromType} → ${toType})`);
          connectingFrom.value = null;
          return;
        }
        
        // 检查输入端口是否已连接
        const existing = edges.find(e => 
          e.to_node_id === node.node_id && e.to_port === port.name
        );
        if (existing) {
          showToast('该输入端口已连接，请先删除现有连接');
          connectingFrom.value = null;
          return;
        }
        
        // 创建连接
        const edgeId = `edge_${Date.now()}`;
        edges.push({
          edge_id: edgeId,
          from_node_id: connectingFrom.value.node_id,
          from_port: connectingFrom.value.port,
          to_node_id: node.node_id,
          to_port: port.name,
        });
        connectingFrom.value = null;
        saveState(); // 保存状态
        scheduleAutoSave(); // 触发自动保存
      }
    }

    // 删除节点
    function deleteNode(nodeId) {
      if (!nodeId) {
        console.warn('deleteNode: nodeId is empty');
        return;
      }
      
      // 确认删除
      if (!confirm('确定要删除这个节点吗？相关的连接也会被删除。')) {
        return;
      }
      
      console.log('Deleting node:', nodeId);
      
      // 删除节点
      const index = nodes.findIndex(n => n.node_id === nodeId);
      if (index !== -1) {
        nodes.splice(index, 1);
        console.log('Node removed from array');
      } else {
        console.warn('Node not found in array:', nodeId);
      }
      
      // 删除相关连接
      const edgesToRemove = edges.filter(e => 
        e.from_node_id === nodeId || e.to_node_id === nodeId
      );
      console.log('Removing edges:', edgesToRemove.length);
      edgesToRemove.forEach(edge => {
        const edgeIndex = edges.findIndex(e => e.edge_id === edge.edge_id);
        if (edgeIndex !== -1) {
          edges.splice(edgeIndex, 1);
        }
      });
      
      // 取消选择
      if (selectedNodeId.value === nodeId) {
        selectedNodeId.value = null;
      }
      
      console.log(`Node ${nodeId} deleted, removed ${edgesToRemove.length} edges`);
      saveState(); // 保存状态
      scheduleAutoSave(); // 触发自动保存
    }

    // 删除连接
    function deleteEdge(edgeId) {
      if (!edgeId) {
        console.warn('deleteEdge: edgeId is empty');
        return;
      }
      
      // 确认删除
      if (!confirm('确定要删除这个连接吗？')) {
        return;
      }
      
      console.log('Deleting edge:', edgeId);
      const index = edges.findIndex(e => e.edge_id === edgeId);
      if (index !== -1) {
        edges.splice(index, 1);
        console.log(`Edge ${edgeId} deleted`);
        // 取消选择
        if (selectedEdgeId.value === edgeId) {
          selectedEdgeId.value = null;
        }
      } else {
        console.warn('Edge not found:', edgeId);
      }
      saveState(); // 保存状态
      scheduleAutoSave(); // 触发自动保存
    }

    // 选中的连接（用于删除）
    const selectedEdgeId = ref(null);

    // 自动保存（防抖）
    function scheduleAutoSave() {
      // 清除之前的定时器
      if (autoSaveTimer) {
        clearTimeout(autoSaveTimer);
      }
      
      // 标记为未保存
      saveStatus.value = 'unsaved';
      
      // 设置新的定时器
      autoSaveTimer = setTimeout(async () => {
        if (currentFlow.value && saveStatus.value === 'unsaved') {
          await autoSaveFlow();
        }
      }, autoSaveInterval);
    }
    
    // 自动保存流程（不显示加载状态）
    async function autoSaveFlow() {
      if (!currentFlow.value) return;
      
      try {
        saveStatus.value = 'saving';
        const flowData = {
          flow_id: currentFlow.value.flow_id,
          name: currentFlow.value.name || "Main Flow",
          description: currentFlow.value.description || "",
          nodes: nodes.map(n => ({
            node_id: n.node_id,
            template_id: n.template_id,
            position: n.position,
            parameters: n.parameters,
          })),
          edges: edges.map(e => ({
            edge_id: e.edge_id,
            from_node_id: e.from_node_id,
            from_port: e.from_port,
            to_node_id: e.to_node_id,
            to_port: e.to_port,
          })),
        };
        
        const response = await fetch(
          getApiUrl(`/projects/${currentProject.value.project_id}/flows/${currentFlow.value.flow_id}`),
          {
            method: "PUT",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(flowData),
          }
        );
        
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        saveStatus.value = 'saved';
        console.log('Auto-saved flow');
      } catch (e) {
        console.error("Failed to auto-save flow", e);
        saveStatus.value = 'error';
        // 3秒后恢复为unsaved状态
        setTimeout(() => {
          if (saveStatus.value === 'error') {
            saveStatus.value = 'unsaved';
          }
        }, 3000);
      }
    }

    // 保存流程
    async function saveFlow() {
      if (!currentFlow.value) {
        // 尝试创建流程
        if (currentProject.value) {
          await createFlow();
          if (!currentFlow.value) {
            showToast("创建流程失败，请检查后端连接", 'error');
            return;
          }
        } else {
          showToast("正在创建项目...", 'info');
          await createProject();
          if (!currentFlow.value) {
            showToast("创建项目和流程失败，请检查后端连接", 'error');
            return;
          }
        }
      }
      
      try {
        isLoading.value = true;
        saveStatus.value = 'saving';
        const flowData = {
          flow_id: currentFlow.value.flow_id,
          name: currentFlow.value.name || "Main Flow",
          description: currentFlow.value.description || "",
          nodes: nodes.map(n => ({
            node_id: n.node_id,
            template_id: n.template_id,
            position: n.position,
            parameters: n.parameters,
          })),
          edges: edges.map(e => ({
            edge_id: e.edge_id,
            from_node_id: e.from_node_id,
            from_port: e.from_port,
            to_node_id: e.to_node_id,
            to_port: e.to_port,
          })),
        };
        
        const response = await fetch(
          getApiUrl(`/projects/${currentProject.value.project_id}/flows/${currentFlow.value.flow_id}`),
          {
            method: "PUT",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(flowData),
          }
        );
        
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        saveStatus.value = 'saved';
        showToast("流程保存成功！", 'success');
      } catch (e) {
        console.error("Failed to save flow", e);
        saveStatus.value = 'error';
        showToast("保存失败: " + e.message, 'error');
        // 3秒后恢复为unsaved状态
        setTimeout(() => {
          if (saveStatus.value === 'error') {
            saveStatus.value = 'unsaved';
          }
        }, 3000);
      } finally {
        isLoading.value = false;
      }
    }

    // 生成代码对话框状态
    const showGenerateDialog = ref(false);
    const generateProjectName = ref("");
    const generateOutputDir = ref("./generated");

    // 生成代码
    async function generateCode() {
      if (!currentFlow.value) {
        // 尝试创建流程
        if (currentProject.value) {
          await createFlow();
          if (!currentFlow.value) {
            showToast("创建流程失败，请检查后端连接", 'error');
            return;
          }
        } else {
          showToast("正在创建项目...", 'info');
          await createProject();
          if (!currentFlow.value) {
            showToast("创建项目和流程失败，请检查后端连接", 'error');
            return;
          }
        }
      }
      
      // 显示对话框
      generateProjectName.value = currentFlow.value.name.replace(/\s+/g, "_").toLowerCase() || "generated_pipeline";
      generateOutputDir.value = "./generated";
      showGenerateDialog.value = true;
    }

    // 确认生成代码
    async function confirmGenerateCode() {
      if (!generateProjectName.value.trim()) {
        showToast("请输入项目名称", 'warning');
        return;
      }
      
      try {
        const response = await fetch(
          getApiUrl(`/projects/${currentProject.value.project_id}/flows/${currentFlow.value.flow_id}/generate`),
          {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
              project_name: generateProjectName.value.trim(),
              output_directory: generateOutputDir.value.trim() || "./generated",
            }),
          }
        );
        
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const data = await response.json();
        
        // 显示生成的代码
        const codeWindow = window.open("", "_blank");
        codeWindow.document.write(`
          <html>
            <head><title>Generated Code - ${data.project_name}</title></head>
            <body style="background: #1a1a1a; color: #f5f5f5; padding: 20px; font-family: monospace;">
              <h1>Generated Project: ${data.project_name}</h1>
              <p>Output Directory: ${data.output_directory}</p>
              <h2>main.cpp</h2>
              <pre style="background: #0f0f0f; padding: 16px; border-radius: 4px; overflow-x: auto;">${escapeHtml(data.files["main.cpp"])}</pre>
              <h2>CMakeLists.txt</h2>
              <pre style="background: #0f0f0f; padding: 16px; border-radius: 4px; overflow-x: auto;">${escapeHtml(data.files["CMakeLists.txt"])}</pre>
            </body>
          </html>
        `);
        
        showGenerateDialog.value = false;
      } catch (e) {
        console.error("Failed to generate code", e);
        showToast("生成代码失败: " + e.message, 'error');
      }
    }

    function escapeHtml(text) {
      const div = document.createElement("div");
      div.textContent = text;
      return div.innerHTML;
    }

    // 获取节点模板
    function getNodeTemplate(templateId) {
      return templates.value.find(t => t.template_id === templateId);
    }

    // 获取端口位置（用于绘制连接线）- 使用计算方式（避免频繁 DOM 查询）
    function getPortPositionReal(nodeId, portName, isOutput) {
      const node = nodes.find(n => n.node_id === nodeId);
      if (!node) return { x: 0, y: 0 };
      
      const template = getNodeTemplate(node.template_id);
      if (!template) return { x: 0, y: 0 };
      
      // 节点尺寸（与 CSS 保持一致）
      const nodePadding = 12;  // padding: 12px
      const nodeHeaderHeight = 30;  // header 高度（包括 margin-bottom: 8px, padding-bottom: 6px）
      const portHeight = 24;  // 每个端口高度（port 高度 + gap: 4px）
      const portDotSize = 10;  // 端口圆点大小
      const portDotMargin = 6;  // 端口圆点 margin
      
      // 计算端口索引
      const ports = isOutput ? template.output_ports : template.input_ports;
      const portIndex = ports.findIndex(p => p.name === portName);
      if (portIndex === -1) return { x: 0, y: 0 };
      
      // 计算端口圆点的 Y 位置（相对于节点顶部）
      // header (30px) + padding (12px) + (端口索引 * 端口高度) + 端口圆点中心 (5px)
      const portY = node.position.y + nodeHeaderHeight + nodePadding + (portIndex * portHeight) + (portDotSize / 2);
      
      // 计算端口圆点的 X 位置
      // 输入端口：节点左边 + padding (12px) + margin (6px) + 圆点中心 (5px) = 23px
      // 输出端口：节点右边 - padding (12px) - margin (6px) - 圆点中心 (5px) = 节点宽度 - 23px
      const portX = isOutput 
        ? node.position.x + 150 - 23  // 输出端口在右侧
        : node.position.x + 23;        // 输入端口在左侧
      
      return { x: portX, y: portY };
    }
    
    // 画布平移
    function onCanvasMouseDown(event) {
      if (event.button !== 0) return;
      // 只有在点击画布背景时才平移，不点击节点、连接线、按钮等
      if (event.target.classList.contains('canvas') || 
          (event.target.classList.contains('canvas-container') && !event.target.closest('.node') && !event.target.closest('svg line'))) {
        isPanning.value = true;
        panStart.value = { x: event.clientX - canvasTransform.x, y: event.clientY - canvasTransform.y };
        event.preventDefault();
        // 取消选择
        selectedNodeId.value = null;
        selectedEdgeId.value = null;
      }
    }
    
    function onCanvasMouseMove(event) {
      if (isPanning.value) {
        canvasTransform.x = event.clientX - panStart.value.x;
        canvasTransform.y = event.clientY - panStart.value.y;
      }
    }
    
    function onCanvasMouseUp(event) {
      if (event.button === 0) {
        isPanning.value = false;
      }
    }
    
    // 画布缩放
    function onCanvasWheel(event) {
      event.preventDefault();
      const delta = event.deltaY > 0 ? 0.9 : 1.1;
      const newScale = Math.max(0.1, Math.min(3, canvasTransform.scale * delta));
      
      // 以鼠标位置为中心缩放
      const rect = event.currentTarget.getBoundingClientRect();
      const mouseX = event.clientX - rect.left;
      const mouseY = event.clientY - rect.top;
      
      const scaleChange = newScale / canvasTransform.scale;
      canvasTransform.x = mouseX - (mouseX - canvasTransform.x) * scaleChange;
      canvasTransform.y = mouseY - (mouseY - canvasTransform.y) * scaleChange;
      canvasTransform.scale = newScale;
    }
    
    // 获取画布样式
    function getCanvasStyle() {
      return {
        transform: `translate(${canvasTransform.x}px, ${canvasTransform.y}px) scale(${canvasTransform.scale})`,
        transformOrigin: '0 0'
      };
    }

    // 撤销/重做功能
    function saveState() {
      // 深拷贝当前状态
      const state = {
        nodes: JSON.parse(JSON.stringify(nodes)),
        edges: JSON.parse(JSON.stringify(edges)),
        timestamp: Date.now()
      };
      
      // 如果当前不在历史记录末尾，删除后面的记录
      if (historyIndex.value < history.value.length - 1) {
        history.value = history.value.slice(0, historyIndex.value + 1);
      }
      
      // 添加新状态
      history.value.push(state);
      
      // 限制历史记录大小
      if (history.value.length > maxHistorySize) {
        history.value.shift();
      } else {
        historyIndex.value = history.value.length - 1;
      }
      
      console.log(`State saved, history size: ${history.value.length}, index: ${historyIndex.value}`);
    }
    
    function applyState(state) {
      if (!state) return;
      
      // 清空当前状态
      nodes.splice(0, nodes.length);
      edges.splice(0, edges.length);
      
      // 应用新状态
      if (state.nodes) {
        nodes.push(...state.nodes);
      }
      if (state.edges) {
        edges.push(...state.edges);
      }
      
      // 取消选择
      selectedNodeId.value = null;
      selectedEdgeId.value = null;
    }
    
    function undo() {
      if (historyIndex.value > 0) {
        historyIndex.value--;
        const state = history.value[historyIndex.value];
        applyState(state);
        console.log(`Undo to index ${historyIndex.value}`);
      } else {
        console.log('No more undo history');
      }
    }
    
    function redo() {
      if (historyIndex.value < history.value.length - 1) {
        historyIndex.value++;
        const state = history.value[historyIndex.value];
        applyState(state);
        console.log(`Redo to index ${historyIndex.value}`);
      } else {
        console.log('No more redo history');
      }
    }
    
    // 检查是否可以撤销/重做
    const canUndo = computed(() => historyIndex.value > 0);
    const canRedo = computed(() => historyIndex.value < history.value.length - 1);

    // 快捷键支持
    function setupKeyboardShortcuts() {
      document.addEventListener('keydown', (e) => {
        // 如果焦点在输入框等元素上，不处理快捷键
        if (e.target.matches('input, textarea, select, button')) {
          return;
        }
        
        // Ctrl+Z / Cmd+Z: 撤销
        if ((e.ctrlKey || e.metaKey) && e.key === 'z' && !e.shiftKey) {
          e.preventDefault();
          undo();
          return;
        }
        
        // Ctrl+Shift+Z / Cmd+Shift+Z 或 Ctrl+Y / Cmd+Y: 重做
        if ((e.ctrlKey || e.metaKey) && ((e.shiftKey && e.key === 'z') || e.key === 'y')) {
          e.preventDefault();
          redo();
          return;
        }
        
        // Delete/Backspace: 删除选中节点或连接
        if (e.key === 'Delete' || e.key === 'Backspace') {
          e.preventDefault();
          if (selectedNodeId.value) {
            deleteNode(selectedNodeId.value);
          } else if (selectedEdgeId.value) {
            deleteEdge(selectedEdgeId.value);
          }
        }
        
        // Escape: 取消选择
        if (e.key === 'Escape') {
          selectedNodeId.value = null;
          selectedEdgeId.value = null;
          connectingFrom.value = null;
        }
      });
    }

    // 生命周期
    onMounted(async () => {
      // 设置快捷键
      setupKeyboardShortcuts();
      
      await loadTemplates();
      await loadProjects();
      // 如果有项目，加载第一个项目；否则创建新项目
      if (projects.value.length > 0) {
        await loadProject(projects.value[0].project_id);
        if (flows.value.length > 0) {
          await loadFlow(flows.value[0].flow_id);
        }
      } else {
        await createProject();
      }
      
      // 初始化历史记录（保存初始状态）
      saveState();
    });

    // 获取类别标签
    function getCategoryLabel(category) {
      const cat = nodeCategories.find(c => c.value === category);
      return cat ? cat.label : category;
    }
    
    return {
      templates,
      filteredTemplates,
      nodes,
      edges,
      selectedNodeId,
      selectedNode,
      connectingFrom,
      currentProject,
      currentFlow,
      projects,
      flows,
      selectedProjectId,
      selectedFlowId,
      isLoading,
      selectedEdgeId,
      notifications,
      showToast,
      removeNotification,
      saveStatus,
      canUndo,
      canRedo,
      undo,
      redo,
      createProject,
      loadProjects,
      loadProject,
      loadFlows,
      loadFlow,
      createNodeFromTemplate,
      deleteNode,
      deleteEdge,
      onTemplateDragStart,
      onCanvasDrop,
      onCanvasDragOver,
      onNodeMouseDown,
      onPortMouseDown,
      onPortMouseUp,
      saveFlow,
      generateCode,
      getNodeTemplate,
      getPortPositionReal,
      canvasTransform,
      onCanvasMouseDown,
      onCanvasMouseMove,
      onCanvasMouseUp,
      onCanvasWheel,
      getCanvasStyle,
      showGenerateDialog,
      generateProjectName,
      generateOutputDir,
      confirmGenerateCode,
      searchKeyword,
      selectedCategory,
      nodeCategories,
      getCategoryLabel,
    };
  },
  template: `
    <div id="app">
      <div class="header">
        <h1>FalconMindBuilder</h1>
        
        <!-- 项目选择器 -->
        <div class="project-selector" style="margin: 0 16px; display: flex; align-items: center; gap: 8px;">
          <label style="font-size: 12px; color: #9fb4ff;">Project:</label>
          <select 
            v-model="selectedProjectId" 
            @change="loadProject(selectedProjectId)"
            style="padding: 4px 8px; background: rgba(0,0,0,0.3); border: 1px solid rgba(255,255,255,0.1); border-radius: 4px; color: #f5f5f5; font-size: 12px; min-width: 150px;"
          >
            <option :value="null">-- Select Project --</option>
            <option v-for="p in projects" :key="p.project_id" :value="p.project_id">{{ p.name }}</option>
          </select>
          <button class="btn" @click="createProject" style="padding: 4px 8px; font-size: 12px;">+ New</button>
        </div>
        
        <!-- 流程选择器 -->
        <div class="flow-selector" style="margin: 0 16px; display: flex; align-items: center; gap: 8px;">
          <label style="font-size: 12px; color: #9fb4ff;">Flow:</label>
          <select 
            v-model="selectedFlowId" 
            @change="loadFlow(selectedFlowId)"
            :disabled="!currentProject"
            style="padding: 4px 8px; background: rgba(0,0,0,0.3); border: 1px solid rgba(255,255,255,0.1); border-radius: 4px; color: #f5f5f5; font-size: 12px; min-width: 150px;"
          >
            <option :value="null">-- Select Flow --</option>
            <option v-for="f in flows" :key="f.flow_id" :value="f.flow_id">{{ f.name }} (v{{ f.version }})</option>
          </select>
          <button class="btn" @click="createFlow" :disabled="!currentProject" style="padding: 4px 8px; font-size: 12px;">+ New</button>
        </div>
        
        <span v-if="isLoading" style="font-size: 12px; color: #9fb4ff; margin: 0 16px;">⏳ Loading...</span>
        
        <!-- 保存状态提示 -->
        <span 
          v-if="currentFlow && !isLoading"
          :style="{
            fontSize: '12px',
            margin: '0 8px',
            color: saveStatus === 'saved' ? '#4caf50' : saveStatus === 'saving' ? '#9fb4ff' : saveStatus === 'error' ? '#ff6b6b' : '#ffa726'
          }"
          :title="saveStatus === 'saved' ? '已保存' : saveStatus === 'saving' ? '保存中...' : saveStatus === 'error' ? '保存失败' : '未保存'"
        >
          {{ saveStatus === 'saved' ? '✓ 已保存' : saveStatus === 'saving' ? '⏳ 保存中...' : saveStatus === 'error' ? '✗ 保存失败' : '● 未保存' }}
        </span>
        
        <!-- 撤销/重做按钮 -->
        <div style="display: flex; gap: 4px; margin: 0 8px;">
          <button 
            class="btn" 
            @click="undo" 
            :disabled="!canUndo"
            title="撤销 (Ctrl+Z)"
            style="padding: 4px 8px; font-size: 12px;"
          >↶ Undo</button>
          <button 
            class="btn" 
            @click="redo" 
            :disabled="!canRedo"
            title="重做 (Ctrl+Shift+Z)"
            style="padding: 4px 8px; font-size: 12px;"
          >↷ Redo</button>
        </div>
        
        <button class="btn btn-primary" @click="saveFlow" :disabled="!currentFlow">Save Flow</button>
        <button class="btn btn-primary" @click="generateCode" :disabled="!currentFlow">Generate Code</button>
      </div>
      
      <div class="main-container">
        <!-- 侧边栏：节点模板库 -->
        <div class="sidebar">
          <div class="sidebar-section">
            <h2>Node Templates</h2>
            
            <!-- 搜索输入框 -->
            <div class="search-box">
              <input
                type="text"
                v-model="searchKeyword"
                placeholder="搜索节点..."
                class="search-input"
              />
            </div>
            
            <!-- 类别过滤下拉框 -->
            <div class="category-filter">
              <select v-model="selectedCategory" class="category-select">
                <option v-for="cat in nodeCategories" :key="cat.value" :value="cat.value">
                  {{ cat.label }}
                </option>
              </select>
            </div>
            
            <!-- 模板列表 -->
            <div class="template-list">
              <div
                v-for="template in filteredTemplates"
                :key="template.template_id"
                class="template-item"
                draggable="true"
                @dragstart="onTemplateDragStart($event, template)"
                :title="template.description || template.name"
              >
                <div class="template-name">{{ template.name }}</div>
                <div class="template-category">{{ getCategoryLabel(template.category) }}</div>
              </div>
              <div v-if="filteredTemplates.length === 0 && templates.length > 0" class="empty-state">
                没有找到匹配的节点
              </div>
              <div v-if="templates.length === 0" class="empty-state">
                Loading templates...
              </div>
            </div>
          </div>
        </div>
        
        <!-- 画布 -->
        <div
          class="canvas-container"
          @drop="onCanvasDrop"
          @dragover="onCanvasDragOver"
          @mousedown="onCanvasMouseDown"
          @mousemove="onCanvasMouseMove"
          @mouseup="onCanvasMouseUp"
          @wheel="onCanvasWheel"
        >
          <div 
            class="canvas"
            :style="getCanvasStyle()"
          >
            <!-- 节点 -->
            <div
              v-for="node in nodes"
              :key="node.node_id"
              :class="['node', { selected: selectedNodeId === node.node_id }]"
              :style="{ left: node.position.x + 'px', top: node.position.y + 'px' }"
              @mousedown="onNodeMouseDown($event, node)"
            >
              <div class="node-header">
                <span>{{ getNodeTemplate(node.template_id)?.name || node.template_id }}</span>
                <button 
                  v-if="selectedNodeId === node.node_id"
                  @click.stop.prevent="deleteNode(node.node_id)"
                  @mousedown.stop
                  class="btn-delete-node"
                  title="删除节点 (Delete键)"
                  style="float: right; background: rgba(255, 77, 77, 0.2); border: 1px solid rgba(255, 77, 77, 0.5); color: #ff6b6b; padding: 2px 6px; font-size: 10px; border-radius: 3px; cursor: pointer; z-index: 100;"
                >×</button>
              </div>
              <div class="node-ports">
                <!-- 输入端口 -->
                <div
                  v-for="port in getNodeTemplate(node.template_id)?.input_ports || []"
                  :key="port.name"
                  class="port port-input"
                >
                  <div
                    class="port-dot"
                    :data-node-id="node.node_id"
                    :data-port-name="port.name"
                    data-is-output="false"
                    @mousedown="onPortMouseDown($event, node, port, false)"
                    @mouseup="onPortMouseUp($event, node, port, false)"
                  ></div>
                  <span>{{ port.name }}</span>
                </div>
                <!-- 输出端口 -->
                <div
                  v-for="port in getNodeTemplate(node.template_id)?.output_ports || []"
                  :key="port.name"
                  class="port port-output"
                >
                  <span>{{ port.name }}</span>
                  <div
                    class="port-dot"
                    :data-node-id="node.node_id"
                    :data-port-name="port.name"
                    data-is-output="true"
                    @mousedown="onPortMouseDown($event, node, port, true)"
                    @mouseup="onPortMouseUp($event, node, port, true)"
                  ></div>
                </div>
              </div>
            </div>
            
            <!-- 连接线（简化版：用 SVG 绘制，放在最后以确保在最上层） -->
            <svg class="connection-line" style="position: absolute; top: 0; left: 0; width: 100%; height: 100%; pointer-events: none; z-index: 10;">
              <!-- 箭头标记定义 -->
              <defs>
                <marker id="arrowhead" markerWidth="10" markerHeight="10" refX="9" refY="3" orient="auto">
                  <polygon points="0 0, 10 3, 0 6" fill="#9fb4ff" />
                </marker>
              </defs>
              <!-- 使用g包裹每个连接，添加更宽的点击区域 -->
              <g v-for="edge in edges" :key="edge.edge_id">
                <!-- 实际显示的连接线 -->
                <line
                  :x1="getPortPositionReal(edge.from_node_id, edge.from_port, true).x"
                  :y1="getPortPositionReal(edge.from_node_id, edge.from_port, true).y"
                  :x2="getPortPositionReal(edge.to_node_id, edge.to_port, false).x"
                  :y2="getPortPositionReal(edge.to_node_id, edge.to_port, false).y"
                  :stroke="selectedEdgeId === edge.edge_id ? '#ff6b6b' : '#9fb4ff'"
                  :stroke-width="selectedEdgeId === edge.edge_id ? 3 : 2"
                  marker-end="url(#arrowhead)"
                  style="pointer-events: stroke; cursor: pointer;"
                  @click.stop="selectedEdgeId = edge.edge_id"
                  @dblclick.stop="deleteEdge(edge.edge_id)"
                  :title="'点击选中，双击删除: ' + edge.from_port + ' → ' + edge.to_port"
                />
                <!-- 更宽的透明点击区域（提高点击成功率） -->
                <line
                  :x1="getPortPositionReal(edge.from_node_id, edge.from_port, true).x"
                  :y1="getPortPositionReal(edge.from_node_id, edge.from_port, true).y"
                  :x2="getPortPositionReal(edge.to_node_id, edge.to_port, false).x"
                  :y2="getPortPositionReal(edge.to_node_id, edge.to_port, false).y"
                  stroke="transparent"
                  stroke-width="10"
                  style="pointer-events: stroke; cursor: pointer;"
                  @click.stop="selectedEdgeId = edge.edge_id"
                  @dblclick.stop="deleteEdge(edge.edge_id)"
                />
              </g>
            </svg>
            
            <div v-if="nodes.length === 0" class="empty-state">
              Drag nodes from the sidebar to create a pipeline
            </div>
          </div>
        </div>
        
        <!-- 属性面板 -->
        <div class="properties-panel" v-if="selectedNode">
          <h2>Node Properties</h2>
          <div class="property-item">
            <label>Node ID</label>
            <input :value="selectedNode.node_id" readonly />
          </div>
          <div class="property-item">
            <label>Template</label>
            <input :value="selectedNode.template_id" readonly />
          </div>
          <div class="property-item">
            <label>Position X</label>
            <input type="number" v-model.number="selectedNode.position.x" />
          </div>
          <div class="property-item">
            <label>Position Y</label>
            <input type="number" v-model.number="selectedNode.position.y" />
          </div>
          
          <!-- 参数配置区域 -->
          <div class="parameters-section" v-if="getNodeTemplate(selectedNode.template_id)">
            <h3>Parameters</h3>
            <div v-if="!getNodeTemplate(selectedNode.template_id).parameters_schema || !getNodeTemplate(selectedNode.template_id).parameters_schema.properties || Object.keys(getNodeTemplate(selectedNode.template_id).parameters_schema.properties).length === 0" class="no-parameters">
              No parameters available for this node.
            </div>
            <div v-else class="parameter-editor">
              <template v-for="(fieldSchema, key) in getNodeTemplate(selectedNode.template_id).parameters_schema.properties" :key="key">
                <!-- 嵌套对象 -->
                <div v-if="fieldSchema.type === 'object' && fieldSchema.properties" class="nested-object">
                  <div class="property-group-header">{{ key }}</div>
                  <div class="property-group">
                    <template v-for="(subSchema, subKey) in fieldSchema.properties" :key="subKey">
                      <div class="property-item" v-if="subSchema.type === 'number'">
                        <label>{{ subKey }} <span class="property-desc">{{ subSchema.description || '' }}</span></label>
                        <input 
                          type="number" 
                          :value="getNestedValue(selectedNode.parameters, key + '.' + subKey) ?? subSchema.default ?? 0" 
                          @input="selectedNode.parameters = setNestedValue(selectedNode.parameters, key + '.' + subKey, parseFloat($event.target.value) || 0)"
                          step="any"
                        />
                      </div>
                      <div class="property-item" v-else-if="subSchema.type === 'string'">
                        <label>{{ subKey }} <span class="property-desc">{{ subSchema.description || '' }}</span></label>
                        <input 
                          type="text" 
                          :value="getNestedValue(selectedNode.parameters, key + '.' + subKey) ?? subSchema.default ?? ''" 
                          @input="selectedNode.parameters = setNestedValue(selectedNode.parameters, key + '.' + subKey, $event.target.value)"
                        />
                      </div>
                      <div class="property-item" v-else-if="subSchema.type === 'boolean'">
                        <label>
                          <input 
                            type="checkbox" 
                            :checked="getNestedValue(selectedNode.parameters, key + '.' + subKey) ?? subSchema.default ?? false" 
                            @change="selectedNode.parameters = setNestedValue(selectedNode.parameters, key + '.' + subKey, $event.target.checked)"
                          />
                          {{ subKey }} <span class="property-desc">{{ subSchema.description || '' }}</span>
                        </label>
                      </div>
                      <div class="property-item" v-else-if="subSchema.enum">
                        <label>{{ subKey }} <span class="property-desc">{{ subSchema.description || '' }}</span></label>
                        <select 
                          :value="getNestedValue(selectedNode.parameters, key + '.' + subKey) ?? subSchema.default" 
                          @change="selectedNode.parameters = setNestedValue(selectedNode.parameters, key + '.' + subKey, $event.target.value)"
                        >
                          <option v-for="opt in subSchema.enum" :key="opt" :value="opt">{{ opt }}</option>
                        </select>
                      </div>
                    </template>
                  </div>
                </div>
                <!-- 普通字段 -->
                <div class="property-item" v-else-if="fieldSchema.type === 'number'">
                  <label>{{ key }} <span class="property-desc">{{ fieldSchema.description || '' }}</span></label>
                  <input 
                    type="number" 
                    :value="selectedNode.parameters[key] ?? fieldSchema.default ?? 0" 
                    @input="selectedNode.parameters[key] = parseFloat($event.target.value) || 0"
                    step="any"
                  />
                </div>
                <div class="property-item" v-else-if="fieldSchema.type === 'string'">
                  <label>{{ key }} <span class="property-desc">{{ fieldSchema.description || '' }}</span></label>
                  <input 
                    type="text" 
                    :value="selectedNode.parameters[key] ?? fieldSchema.default ?? ''" 
                    @input="selectedNode.parameters[key] = $event.target.value"
                  />
                </div>
                <div class="property-item" v-else-if="fieldSchema.type === 'boolean'">
                  <label>
                    <input 
                      type="checkbox" 
                      :checked="selectedNode.parameters[key] ?? fieldSchema.default ?? false" 
                      @change="selectedNode.parameters[key] = $event.target.checked"
                    />
                    {{ key }} <span class="property-desc">{{ fieldSchema.description || '' }}</span>
                  </label>
                </div>
                <div class="property-item" v-else-if="fieldSchema.enum">
                  <label>{{ key }} <span class="property-desc">{{ fieldSchema.description || '' }}</span></label>
                  <select 
                    :value="selectedNode.parameters[key] ?? fieldSchema.default" 
                    @change="selectedNode.parameters[key] = $event.target.value"
                  >
                    <option v-for="opt in fieldSchema.enum" :key="opt" :value="opt">{{ opt }}</option>
                  </select>
                </div>
                <div class="property-item" v-else-if="fieldSchema.type === 'array'">
                  <label>{{ key }} <span class="property-desc">{{ fieldSchema.description || '' }}</span></label>
                  <div class="array-editor">
                    <div v-for="(item, index) in (selectedNode.parameters[key] || [])" :key="index" class="array-item">
                      <input 
                        type="text" 
                        :value="item" 
                        @input="selectedNode.parameters[key][index] = $event.target.value"
                      />
                      <button type="button" class="btn-remove" @click="selectedNode.parameters[key].splice(index, 1)">×</button>
                    </div>
                    <button type="button" class="btn-add" @click="(selectedNode.parameters[key] = selectedNode.parameters[key] || []).push('')">+ Add</button>
                  </div>
                </div>
              </template>
            </div>
          </div>
        </div>
      </div>
      
      <!-- 生成代码对话框 -->
      <div v-if="showGenerateDialog" style="position: fixed; top: 0; left: 0; right: 0; bottom: 0; background: rgba(0,0,0,0.7); z-index: 1000; display: flex; align-items: center; justify-content: center;">
        <div style="background: #1a1a1a; padding: 24px; border-radius: 8px; min-width: 400px; border: 2px solid #9fb4ff;">
          <h2 style="margin: 0 0 16px 0; color: #9fb4ff;">Generate Code</h2>
          <div style="margin-bottom: 12px;">
            <label style="display: block; margin-bottom: 4px; color: #a0a8c0; font-size: 12px;">Project Name:</label>
            <input v-model="generateProjectName" type="text" style="width: 100%; padding: 8px; background: rgba(0,0,0,0.3); border: 1px solid rgba(255,255,255,0.1); border-radius: 4px; color: #f5f5f5;" />
          </div>
          <div style="margin-bottom: 16px;">
            <label style="display: block; margin-bottom: 4px; color: #a0a8c0; font-size: 12px;">Output Directory:</label>
            <input v-model="generateOutputDir" type="text" style="width: 100%; padding: 8px; background: rgba(0,0,0,0.3); border: 1px solid rgba(255,255,255,0.1); border-radius: 4px; color: #f5f5f5;" />
          </div>
          <div style="display: flex; gap: 8px; justify-content: flex-end;">
            <button class="btn" @click="showGenerateDialog = false">Cancel</button>
            <button class="btn btn-primary" @click="confirmGenerateCode">Generate</button>
          </div>
        </div>
      </div>
    </div>
  `,
}).mount("#app");
