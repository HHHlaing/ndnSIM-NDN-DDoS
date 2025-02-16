#!/usr/bin/env Rscript

# install.packages('ggplot2')
library(ggplot2)
# install.packages('scales')
library(scales)
# install.packages('doBy')
library(doBy)

#########################
# Rate trace processing #
#########################
args = commandArgs(trailingOnly=TRUE)
target = args[1]
fake_threshold = args[2]
valid_threshold = args[3]
#target = "mixed-f100-c1500-t500"
#fake_threshold = 500
#valid_threshold = 1500
validAmount = 240

data = read.table(paste(target, "txt", sep="."), header=T)
data$Node = factor(data$Node)
data$FaceId <- factor(data$FaceId)
data$Kilobits <- data$Kilobytes * 8
data$Type = factor(data$Type)

# exlude irrelevant types
data = subset(data, Type %in% c("InInterests"))

data.consumer = subset(data, Node %in% c("as1-math-g0",
                                         "as1-math-g1",
                                         "as2-math-g0",
                                         "as2-math-g1",
                                         "as2-cs-g0",
                                         "as2-cs-g1",
                                         "as3-cs-g0",
                                         "as3-cs-g1",
                                         "as4-hw-g0",
                                         "as4-hw-g1",
                                         "as4-sm-g0",
                                         "as4-sm-g1"))

data.attacker = subset(data, Node %in% c("as1-math-a0",
                                         "as1-math-a1",
                                         "as1-math-a2",
                                         "as1-math-a3",
                                         "as1-math-a4",
                                         "as1-math-a5",
                                         "as1-math-a6",
                                         "as1-math-a7",
                                         "as1-math-a8",
                                         "as1-math-a9",
                                         "as2-math-a0",
                                         "as2-math-a1",
                                         "as2-math-a2",
                                         "as2-math-a3",
                                         "as2-math-a4",
                                         "as2-math-a5",
                                         "as2-math-a6",
                                         "as2-math-a7",
                                         "as2-math-a8",
                                         "as2-math-a9",
                                         "as2-cs-a0",
                                         "as2-cs-a1",
                                         "as2-cs-a2",
                                         "as2-cs-a3",
                                         "as2-cs-a4",
                                         "as2-cs-a5",
                                         "as2-cs-a6",
                                         "as2-cs-a7",
                                         "as2-cs-a8",
                                         "as2-cs-a9",
                                         "as3-cs-a0",
                                         "as3-cs-a1",
                                         "as3-cs-a2",
                                         "as3-cs-a3",
                                         "as3-cs-a4",
                                         "as3-cs-a5",
                                         "as3-cs-a6",
                                         "as3-cs-a7",
                                         "as3-cs-a8",
                                         "as3-cs-a9",
                                         "as4-hw-a0",
                                         "as4-hw-a1",
                                         "as4-hw-a2",
                                         "as4-hw-a3",
                                         "as4-hw-a4",
                                         "as4-hw-a5",
                                         "as4-hw-a6",
                                         "as4-hw-a7",
                                         "as4-hw-a8",
                                         "as4-hw-a9",
                                         "as4-sm-a0",
                                         "as4-sm-a1",
                                         "as4-sm-a2",
                                         "as4-sm-a3",
                                         "as4-sm-a4",
                                         "as4-sm-a5",
                                         "as4-sm-a6",
                                         "as4-sm-a7",
                                         "as4-sm-a8",
                                         "as4-sm-a9"))

data.victim = subset(data, Node %in% c("as1-cs-server"))
data.gateway = subset(data, Node %in% c("as1-cs"))

# combine stats from all faces
data = summaryBy(. ~ Time + Node + Type, data=data, FUN=sum)
data.attacker = summaryBy(. ~ Time + Type, data=data.attacker, FUN=sum)
data.victim = summaryBy(. ~ Time + Type, data=data.victim, FUN=sum)
data.gateway = summaryBy(. ~ Time + Type, data=data.gateway, FUN=sum)
data.consumer = summaryBy(. ~ Time + Type, data=data.consumer, FUN=sum)
data.attacker$Node = "attacker"
data.victim$Node = "victim"
data.gateway$Node = "gateway"
data.consumer$Node = "consumer"
result = rbind(data.victim, data.attacker, data.consumer)

# graph rates on selected nodes in number of incoming interest packets
labelstr = paste(c("Tolerance (", fake_threshold, ") + Capacity (", valid_threshold, ")"), collapse = "")
g.nodes <- ggplot(result, aes (x=Time, y=Packets.sum)) +
  scale_shape_manual(values=c(8, 16, 15)) +
  scale_color_manual(values=c("red","darkgreen", "deepskyblue")) +
  geom_line(aes (x=Time, y=Packets.sum, color=Node, linetype=Node, alpha=Node), size=1.5) +
  scale_alpha_manual(values=c(1, 1, 0.7)) +
  scale_linetype_manual(values=c("dashed", "dotdash", "solid")) +
  geom_hline(yintercept = as.numeric(fake_threshold) + as.numeric(valid_threshold), linetype="longdash", size=1.3) + 
  annotate("text", x = 15, y = 350 + as.numeric(valid_threshold), label = "RPS=1500", size = 11) +
  xlab("Time [second]") +
  ylab("Rate [Interest / second]") +
  theme_linedraw() +
  theme(
    legend.position="none", text = element_text(size=12),
    axis.text.x = element_text(color = "grey20", size = 33, angle = 0, face = "plain"),
    axis.text.y = element_text(color = "grey20", size = 33, angle = 90, face = "plain"),
    axis.title.x = element_text(color="black", size=33, face="bold"),
    axis.title.y = element_text(color="black", size=33, face="bold")
  )

png(paste(target, "png", sep="."), width=500, height=500)
print(g.nodes)
retval <- dev.off()
