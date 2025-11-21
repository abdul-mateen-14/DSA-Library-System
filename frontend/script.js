const apiUrl = "https://your-railway-app.railway.app"; // Replace with your Railway backend URL

function showTab(tabName) {
    const contents = document.querySelectorAll('.tab-content');
    contents.forEach(content => content.classList.add('hidden'));
    document.getElementById(tabName + '-content').classList.remove('hidden');
    
    const buttons = document.querySelectorAll('.tab-btn');
    buttons.forEach(btn => {
        btn.classList.remove('bg-blue-600', 'text-white');
        btn.classList.add('text-gray-600', 'hover:text-blue-600');
    });
    
    document.getElementById('tab-' + tabName).classList.add('bg-blue-600', 'text-white');
    document.getElementById('tab-' + tabName).class
