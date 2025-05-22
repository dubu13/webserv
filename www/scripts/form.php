<?php
// Form handling PHP script
$name = isset($_POST['name']) ? $_POST['name'] : (isset($_GET['name']) ? $_GET['name'] : '');
$email = isset($_POST['email']) ? $_POST['email'] : (isset($_GET['email']) ? $_GET['email'] : '');
$method = $_SERVER['REQUEST_METHOD'];

header('Content-Type: text/html');
?>
<!DOCTYPE html>
<html>
<head>
    <title>PHP Form Example</title>
    <style>
        body { font-family: sans-serif; margin: 20px; max-width: 800px; }
        form { background: #f5f5f5; padding: 20px; border-radius: 8px; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; }
        input[type="text"], input[type="email"] { width: 100%; padding: 8px; box-sizing: border-box; }
        button { background: #4CAF50; color: white; border: none; padding: 10px 15px; cursor: pointer; }
    </style>
</head>
<body>
    <h1>PHP Form Processing</h1>
    
    <?php if ($name && $email): ?>
        <div style="background: #dff0d8; padding: 15px; margin-bottom: 20px; border-radius: 4px;">
            <h2>Form Submitted Successfully!</h2>
            <p>Request method used: <strong><?php echo htmlspecialchars($method); ?></strong></p>
            <p>Name: <strong><?php echo htmlspecialchars($name); ?></strong></p>
            <p>Email: <strong><?php echo htmlspecialchars($email); ?></strong></p>
        </div>
    <?php endif; ?>

    <h2>Submit Form (GET method)</h2>
    <form action="/scripts/form.php" method="get">
        <div class="form-group">
            <label for="name">Name:</label>
            <input type="text" id="name" name="name" required>
        </div>
        <div class="form-group">
            <label for="email">Email:</label>
            <input type="email" id="email" name="email" required>
        </div>
        <button type="submit">Submit with GET</button>
    </form>

    <h2>Submit Form (POST method)</h2>
    <form action="/scripts/form.php" method="post">
        <div class="form-group">
            <label for="post-name">Name:</label>
            <input type="text" id="post-name" name="name" required>
        </div>
        <div class="form-group">
            <label for="post-email">Email:</label>
            <input type="email" id="post-email" name="email" required>
        </div>
        <button type="submit">Submit with POST</button>
    </form>
</body>
</html>