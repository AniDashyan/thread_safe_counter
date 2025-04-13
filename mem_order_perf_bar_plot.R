if (!require(ggplot2)) install.packages("ggplot2")

library(ggplot2)

data <- read.csv("results.csv")

# Bar plot
ggplot(data, aes(x = MemoryOrder, y = DurationMs, fill = MemoryOrder)) +
  geom_bar(stat = "identity") +
  geom_text(aes(label = sprintf("%d ms\n(Counter: %d)", DurationMs, CounterValue)), vjust = -0.5, size = 3.5) +
  labs(title = "Execution Time by Memory Order",
       x = "Memory Order",
       y = "Time (ms)") +
  theme_minimal() +
  theme(legend.position = "none")