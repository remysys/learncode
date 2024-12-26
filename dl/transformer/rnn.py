import torch
import torch.nn as nn
import torch.optim as optim

# define RNN model


class RNNModel(nn.Module):
  def __init__(self, input_size, hidden_size, output_size):
    super().__init__()
    self.rnn = nn.RNN(input_size, hidden_size, batch_first=True)
    self.fc = nn.Linear(hidden_size, output_size)

  def forward(self, x):
    # x: (batch_size, seq_length, input_size)
    out, _ = self.rnn(x)
    # take the output of the last time step
    out = self.fc(out[:, -1, :])
    return out


# hyperparameters
input_size = 10   # dimension of input features
hidden_size = 20  # dimension of hidden layer
output_size = 1   # output dimension (e.g., for binary classification)
num_epochs = 100  # number of training epochs
learning_rate = 0.01

# create model, loss function, and optimizer
model = RNNModel(input_size, hidden_size, output_size)
criterion = nn.MSELoss()  # for regression task
optimizer = optim.Adam(model.parameters(), lr=learning_rate)

# training data example
# assume input is 100 samples, each with 5 time steps, each time step has 10 features
X_train = torch.randn(100, 5, input_size)
y_train = torch.randn(100, output_size)

# training loop
for epoch in range(num_epochs):
  model.train()  # set to training mode
  optimizer.zero_grad()  # clear gradients

  # forward pass
  outputs = model(X_train)
  loss = criterion(outputs, y_train)  # compute loss

  # backward pass and optimize
  loss.backward()
  optimizer.step()

  if (epoch + 1) % 10 == 0:  # print loss every 10 epochs
    print(f'epoch [{epoch + 1}/{num_epochs}], loss: {loss.item():.4f}')

# test the model
model.eval()  # set to evaluation mode
with torch.no_grad():
  test_input = torch.randn(1, 5, input_size)  # single test sample
  test_output = model(test_input)
  print("Test output:", test_output)
